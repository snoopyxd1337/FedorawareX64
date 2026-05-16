#include "../Hooks.h"

#include"../../Features/NoSpread/NoSpread.h"
#include "../../Features/TickHandler/TickHandler.h"

namespace
{
	int GetSafeCLSendMoveChokedCommands()
	{
		if (I::EngineClient)
		{
			if (const auto netChannel = I::EngineClient->GetNetChannelInfo())
			{
				const int value = netChannel->m_nChokedPackets;
				if (value >= 0 && value <= 21)
				{
					return value;
				}
			}
		}

		if (I::ClientState)
		{
			const int value = I::ClientState->chokedcommands;
			if (value >= 0 && value <= 21)
			{
				return value;
			}
		}

		return 0;
	}

	void CLSendMoveTraceEvery(const uint64_t throttleMs, const std::string& message)
	{
		if (!Vars::Debug::TickbaseLogging.Value)
		{
			return;
		}

		static uint64_t s_LastTraceMs = 0;
		const uint64_t now = GetTickCount64();
		if (now - s_LastTraceMs < throttleMs)
		{
			return;
		}

		s_LastTraceMs = now;
		FW_TRACE(message.c_str());
	}
}

MAKE_HOOK(CL_SendMove, S::CL_SendMove(), void, __cdecl)
{
	F::NoSpread.AskForPlayerPerf();

	if (!I::ClientState)
	{
		CLSendMoveTraceEvery(250, "[TICKDBG][ERROR] clsendmove missing ClientState");
		return;
	}

	byte data[4000];

	const int chokedCommands = GetSafeCLSendMoveChokedCommands();
	const int nextcommandnr = I::ClientState->lastoutgoingcommand + chokedCommands + 1;

	CLC_Move moveMsg;
	moveMsg.m_DataOut.StartWriting(data, sizeof(data));
	moveMsg.m_nNewCommands = std::clamp(1 + chokedCommands, 0, 15);
	const int extraCommands = chokedCommands + 1 - moveMsg.m_nNewCommands;
	const int backupCommands = std::max(2, extraCommands);
	moveMsg.m_nBackupCommands = std::clamp(backupCommands, 0, 7);

	const int numcmds = moveMsg.m_nNewCommands + moveMsg.m_nBackupCommands;
	CLSendMoveTraceEvery(250, std::format("[TICKDBG] clsendmove choked={} lastOut={} next={} new={} backup={} extra={} shifted={} wait={} tickrate={} maxprocess={} maxshift={}",
		chokedCommands,
		I::ClientState->lastoutgoingcommand,
		nextcommandnr,
		moveMsg.m_nNewCommands,
		moveMsg.m_nBackupCommands,
		extraCommands,
		G::ShiftedTicks,
		G::WaitForShift,
		F::Ticks.GetTickRate(),
		F::Ticks.GetMaxUserCmdProcessTicks(),
		F::Ticks.GetMaxShift()));

	int from = -1;
	bool bOK = true;
	for (int to = nextcommandnr - numcmds + 1; to <= nextcommandnr; to++)
	{
		const bool isnewcmd = to >= nextcommandnr - moveMsg.m_nNewCommands + 1;
		bOK = bOK && I::BaseClientDLL->WriteUsercmdDeltaToBuffer(&moveMsg.m_DataOut, from, to, isnewcmd);
		from = to;
	}

	if (bOK)
	{
		if (extraCommands)
		{
			if (auto netChannel = I::EngineClient->GetNetChannelInfo())
			{
				netChannel->m_nChokedPackets -= extraCommands;
			}
		}
		GetVFunc<bool(__thiscall*)(PVOID, INetMessage* msg, bool, bool)>(I::ClientState->m_NetChannel, 37)(I::ClientState->m_NetChannel, &moveMsg, false, false);
	}
}
