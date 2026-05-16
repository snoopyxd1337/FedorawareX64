#include "../Hooks.h"
#include "../../Features/TickHandler/TickHandler.h"

namespace
{
	bool IsSaneChokedCommands(const int value)
	{
		return value >= 0 && value <= 21;
	}

	INetChannel* GetWritableNetChannel(CNetChannel* netChannel)
	{
		if (I::EngineClient)
		{
			if (auto engineNetChannel = I::EngineClient->GetNetChannelInfo())
			{
				return engineNetChannel;
			}
		}

		return reinterpret_cast<INetChannel*>(netChannel);
	}

	bool TryGetSafeChokedCommands(CNetChannel* netChannel, int& outChoked, int& rawClientChoked, int& rawNetChoked)
	{
		rawClientChoked = I::ClientState ? I::ClientState->chokedcommands : -1;
		rawNetChoked = -1;

		if (const auto writableNetChannel = GetWritableNetChannel(netChannel))
		{
			rawNetChoked = writableNetChannel->m_nChokedPackets;
			if (IsSaneChokedCommands(rawNetChoked))
			{
				outChoked = rawNetChoked;
				return true;
			}
		}

		if (IsSaneChokedCommands(rawClientChoked))
		{
			outChoked = rawClientChoked;
			return true;
		}

		outChoked = 0;
		return false;
	}

	bool TickNetTraceEnabled()
	{
		return Vars::Debug::TickbaseLogging.Value;
	}

	void TickNetTraceEvery(const int slot, const uint64_t throttleMs, const std::string& message)
	{
		if (!TickNetTraceEnabled())
		{
			return;
		}

		static uint64_t s_LastTraceMs[8] = {};
		const int safeSlot = std::clamp(slot, 0, 7);
		const uint64_t now = GetTickCount64();
		if (now - s_LastTraceMs[safeSlot] < throttleMs)
		{
			return;
		}

		s_LastTraceMs[safeSlot] = now;
		FW_TRACE(message.c_str());
	}

	void TickNetTraceEvent(const std::string& message)
	{
		if (TickNetTraceEnabled())
		{
			FW_TRACE(message.c_str());
		}
	}
}

//	"NetMsg"
//	@net_chan.cpp L2524
MAKE_HOOK(CNetChan_SendNetMsg, S::CNetChan_SendNetMsg(), bool, __fastcall,
		  CNetChannel* netChannel, INetMessage& msg, bool bForceReliable, bool bVoice)
{
	switch (msg.GetType())
	{
		case clc_VoiceData:
		{
			// stop lag with voice chat
			bVoice = true;
			break;
		}

		case clc_FileCRCCheck:
		{
			// whitelist
			if (Vars::Misc::BypassPure.Value)
			{
				return false;
			}
			break;
		}

		case clc_RespondCvarValue:
		{
			//	causes b1g crash
			if (Vars::Visuals::RemoveConvarQueries.Value)
			{
				if (const auto respondMsg = reinterpret_cast<uintptr_t*>(&msg))
				{
					if (const auto cvarName = reinterpret_cast<const char*>(respondMsg[6]))
					{
						if (const auto convarC = g_ConVars.FindVar(cvarName))
						{
							if (const char* defaultValue = convarC->GetDefault())
							{
								respondMsg[7] = reinterpret_cast<uintptr_t>(defaultValue);
								I::Cvar->ConsoleColorPrintf({ 255, 0, 0, 255 }, "%s\n", msg.ToString()); //	mt everest
								break;
							}
						}
						return true; //	if we failed to manipulate the data, don't send it.
					}
				}
			}
			break;
		}

		case clc_Move:
		{
			if (F::Ticks.bIgnoreSendNetMsg) { break; }
			const auto moveMsg = reinterpret_cast<CLC_Move*>(&msg);
			if (!moveMsg) { break; }
			if (!I::ClientState)
			{
				TickNetTraceEvery(2, 250, "[TICKDBG][ERROR] sendnet missing ClientState; fallback original");
				break;
			}

			const int nOriginalNewCommands = moveMsg->m_nNewCommands;
			const int nOriginalBackupCommands = moveMsg->m_nBackupCommands;
			const int nLastOutgoingCommand = I::ClientState->lastoutgoingcommand;
			int nRawClientChokedCommands = -1;
			int nRawNetChokedPackets = -1;
			int nChokedCommands = 0;
			if (!TryGetSafeChokedCommands(netChannel, nChokedCommands, nRawClientChokedCommands, nRawNetChokedPackets))
			{
				TickNetTraceEvery(1, 250, std::format("[TICKDBG][ERROR] insane choked counters fallback original clientChoked={} netChoked={} shifted={} old={}/{} lastOut={}",
					nRawClientChokedCommands, nRawNetChokedPackets, G::ShiftedTicks, nOriginalNewCommands, nOriginalBackupCommands, nLastOutgoingCommand));
				break;
			}

			const int nNextCommandNr = nLastOutgoingCommand + nChokedCommands + 1;
			const int nNetChokedBefore = nRawNetChokedPackets;
			const bool bTickbaseActive = F::Ticks.IsTickbaseActive() || G::ShouldShift || G::Teleporting || G::Recharging;
			TickNetTraceEvery(2, 250, std::format("[TICKDBG] sendnet input old={}/{} clientChoked={} netChoked={} chosenChoked={} lastOut={} next={} tickbaseActive={} shifted={} wait={} tickrate={} maxprocess={} maxshift={} forceRel={} voice={}",
				nOriginalNewCommands, nOriginalBackupCommands,
				nRawClientChokedCommands, nRawNetChokedPackets, nChokedCommands,
				nLastOutgoingCommand, nNextCommandNr,
				bTickbaseActive,
				G::ShiftedTicks, G::WaitForShift,
				F::Ticks.GetTickRate(), F::Ticks.GetMaxUserCmdProcessTicks(), F::Ticks.GetMaxShift(),
				bForceReliable, bVoice));

			if (!bTickbaseActive)
			{
				TickNetTraceEvery(4, 250, std::format("[TICKDBG] sendnet passthrough old={}/{} clientChoked={} netChoked={} chosenChoked={} lastOut={} shifted={} wait={}",
					nOriginalNewCommands, nOriginalBackupCommands,
					nRawClientChokedCommands, nRawNetChokedPackets, nChokedCommands,
					nLastOutgoingCommand, G::ShiftedTicks, G::WaitForShift));
				break;
			}

			byte data[4000];
			CLC_Move rebuiltMsg;
			rebuiltMsg.m_DataOut.StartWriting(data, sizeof(data));

			const int nCommands = 1 + nChokedCommands;
			rebuiltMsg.m_nNewCommands = std::clamp(nCommands, 0, MAX_NEW_COMMANDS);
			const int nExtraCommands = nCommands - rebuiltMsg.m_nNewCommands;
			rebuiltMsg.m_nBackupCommands = std::clamp(nExtraCommands, 2, MAX_BACKUP_COMMANDS);

			bool bOk = true;
			const int nNumCmds = rebuiltMsg.m_nNewCommands + rebuiltMsg.m_nBackupCommands;
			for (int nFrom = -1, nTo = nNextCommandNr - nNumCmds + 1; nTo <= nNextCommandNr; nTo++)
			{
				const bool bIsNewCmd = nTo >= nNextCommandNr - rebuiltMsg.m_nNewCommands + 1;
				bOk = bOk && I::BaseClientDLL->WriteUsercmdDeltaToBuffer(&rebuiltMsg.m_DataOut, nFrom, nTo, bIsNewCmd);
				nFrom = nTo;
			}

			if (!bOk)
			{
				TickNetTraceEvent(std::format("[TICKDBG][ERROR] sendnet rebuild failed fallback original oldNew={} oldBackup={} choked={} shifted={} lastOut={} next={}",
					nOriginalNewCommands, nOriginalBackupCommands, nChokedCommands, G::ShiftedTicks, nLastOutgoingCommand, nNextCommandNr));
				break;
			}

			if (nExtraCommands > 0 && netChannel)
			{
				if (auto writableNetChannel = GetWritableNetChannel(netChannel))
				{
					TickNetTraceEvery(3, 100, std::format("[TICKDBG] sendnet extra commands extra={} netChokedBefore={} netChokedAfter={} new={} backup={} shifted={} maxprocess={}",
						nExtraCommands, writableNetChannel->m_nChokedPackets, writableNetChannel->m_nChokedPackets - nExtraCommands,
						rebuiltMsg.m_nNewCommands, rebuiltMsg.m_nBackupCommands, G::ShiftedTicks, F::Ticks.GetMaxUserCmdProcessTicks()));
					writableNetChannel->m_nChokedPackets -= nExtraCommands;
				}
			}

			const bool bSent = Hook.Original<FN>()(netChannel, reinterpret_cast<INetMessage&>(rebuiltMsg), bForceReliable, bVoice);

			if (!F::Ticks.bIgnoreSendNetMsg)
			{
				const int iTicksMax = g_ConVars.sv_maxusrcmdprocessticks ? g_ConVars.sv_maxusrcmdprocessticks->GetInt() : 24;
				const int iAllowedNewCommands = std::max(iTicksMax - G::ShiftedTicks, 0);
				const int iCmdCount = std::max(0, rebuiltMsg.m_nNewCommands + rebuiltMsg.m_nBackupCommands - 3);
				if (iCmdCount > iAllowedNewCommands)
				{
					F::Ticks.iDeficit = iCmdCount - iAllowedNewCommands;
					if (Vars::Debug::Logging.Value)
					{
						Utils::ConLog("clc_Move", std::format("{:d} sent <{:d} | {:d}>, max was {:d}.",
							iCmdCount + 3, rebuiltMsg.m_nNewCommands, rebuiltMsg.m_nBackupCommands, iAllowedNewCommands).c_str(), { 0, 222, 255, 255 }, true);
					}

					static int nLoggedDeficit = 0;
					if (nLoggedDeficit++ < 3)
					{
						FW_TRACE(std::format("[CL_MOVE] deficit cmd_count={} new={} backup={} allowed={} shifted={} max={}",
							iCmdCount, rebuiltMsg.m_nNewCommands, rebuiltMsg.m_nBackupCommands, iAllowedNewCommands, G::ShiftedTicks, iTicksMax).c_str());
					}
				}
				F::Ticks.iPredicted -= iCmdCount;

				TickNetTraceEvery(0, 100, std::format("[TICKDBG] sendnet sent={} old={}/{} new={}/{} extra={} cmdCount={} allowed={} lastOut={} choked={} clientChoked={} netChoked={}->{} shifted={} wait={} flags(dt={} tp={} rc={}) active={} tickrate={} maxprocess={} maxshift={} deficit={} pred={}",
					bSent,
					nOriginalNewCommands, nOriginalBackupCommands,
					rebuiltMsg.m_nNewCommands, rebuiltMsg.m_nBackupCommands,
					nExtraCommands, iCmdCount, iAllowedNewCommands,
					nLastOutgoingCommand, nChokedCommands, nRawClientChokedCommands,
					nNetChokedBefore, GetWritableNetChannel(netChannel) ? GetWritableNetChannel(netChannel)->m_nChokedPackets : -1,
					G::ShiftedTicks, G::WaitForShift,
					G::ShouldShift, G::Teleporting, G::Recharging,
					bTickbaseActive,
					F::Ticks.GetTickRate(), F::Ticks.GetMaxUserCmdProcessTicks(), F::Ticks.GetMaxShift(),
					F::Ticks.iDeficit, F::Ticks.iPredicted));
			}
			return bSent;
		}
	}

	return Hook.Original<FN>()(netChannel, msg, bForceReliable, bVoice);
}
