#include "../Hooks.h"

#include "../../Features/Prediction/Prediction.h"
#include "../../Features/Aimbot/Aimbot.h"
#include "../../Features/Auto/Auto.h"
#include "../../Features/Misc/Misc.h"
#include "../../Features/Visuals/Visuals.h"
#include "../../Features/Backtrack/Backtrack.h"
#include "../../Features/Visuals/FakeAngleManager/FakeAng.h"
#include "../../Features/Camera/CameraWindow.h"
#include "../../Features/CritHack/CritHack.h"
#include "../../Features/Fedworking/Fedworking.h"
#include "../../Features/Resolver/Resolver.h"
#include "../../Features/AntiHack/CheaterDetection/CheaterDetection.h"
#include "../../Features/Followbot/Followbot.h"
#include "../../Features/Vars.h"
#include "../../Features/Aimbot/AimbotGlobal/AimbotGlobal.h"
#include "../../Features/Menu/MaterialEditor/MaterialEditor.h"
#include "../../Features/TickHandler/TickHandler.h"
#include "../../Features/Backtrack/Backtrack.h"
#include "../../Features/NoSpread/NoSpread.h"
#include "../../Features/PacketManip/PacketManip.h"
#include "../../Utils/FunctionalDebug/FunctionalDebug.h"

#include <intrin.h>

namespace
{
	int GetSafeCreateMoveChokedCommands()
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

	void UpdateVerifiedUserCmd(const int sequence_number, const CUserCmd* pCmd)
	{
		if (!I::Input || !I::Input->m_pVerifiedCommands || !pCmd)
		{
			return;
		}

		auto pVerifiedCommands = reinterpret_cast<CVerifiedUserCmd*>(I::Input->m_pVerifiedCommands);
		CVerifiedUserCmd& verifiedCmd = pVerifiedCommands[sequence_number % MULTIPLAYER_BACKUP];
		verifiedCmd.m_cmd = *pCmd;
		verifiedCmd.m_crc = pCmd->GetChecksum();

		static int nLoggedVerified = 0;
		if (Vars::Debug::TickbaseLogging.Value && nLoggedVerified++ < 8)
		{
			FW_TRACE(std::format("[TICKDBG] verified cmd={} crc=0x{:X} move={:.1f}/{:.1f}",
				pCmd->command_number, verifiedCmd.m_crc, pCmd->forwardmove, pCmd->sidemove).c_str());
		}
	}

	void UpdateWeaponAttackState(CBaseEntity* pLocal, CBaseCombatWeapon* pWeapon, CUserCmd* pCmd)
	{
		if (!pLocal || !pWeapon)
		{
			return;
		}

		G::WeaponCanAttack = pWeapon->CanShoot(pLocal);
		G::WeaponCanSecondaryAttack = pWeapon->CanSecondaryAttack(pLocal);

		if (pWeapon->GetSlot() != SLOT_MELEE)
		{
			if (G::CurItemDefIndex != Soldier_m_TheBeggarsBazooka && pWeapon->GetClip1() == 0)
			{
				G::WeaponCanAttack = false;
			}
		}

		G::WeaponCanHeadShot = pWeapon->CanWeaponHeadShot();
		G::CurWeaponType = Utils::GetWeaponType(pWeapon);
		F::AimbotGlobal.SetAttacking(Utils::IsAttacking(pCmd, pWeapon));
	}
}

MAKE_HOOK(BaseClientDLL_CreateMove, Utils::GetVFuncPtr(I::BaseClientDLL, 21), void, __fastcall,
		  void* ecx, int sequence_number, float input_sample_frametime, bool active)
{
	FDBG_FLUSH("CreateMove");
	FDBG_SCOPE_SLOW("Hook.BaseClientDLL_CreateMove", FunctionalDebug::HOOK_SLOW_US);

	FDBG_CALL("CreateMove.Original", Hook.Original<FN>()(ecx, sequence_number, input_sample_frametime, active));

	G::UpdateView = true;
	G::SilentTime = false;
	F::AimbotGlobal.SetAttacking(false);

	if (!I::Input || !I::Input->m_pCommands)
	{
		FDBG_SKIP("CreateMove.Features", "I::Input or command buffer null");
		static int nLoggedMissingInput = 0;
		if (nLoggedMissingInput++ < 5)
		{
			FW_TRACE("[CUSERCMD] BaseClientDLL_CreateMove: input command buffer is null");
		}
		return;
	}

	CUserCmd* pCmd = &I::Input->m_pCommands[sequence_number % MULTIPLAYER_BACKUP];
	if (!pCmd || !pCmd->command_number)
	{
		FDBG_SKIP("CreateMove.Features", "CUserCmd null or command_number zero");
		static int nLoggedInvalidCmd = 0;
		if (nLoggedInvalidCmd++ < 5)
		{
			const auto message = std::format("[CUSERCMD] BaseClientDLL_CreateMove: invalid cmd seq={} ptr=0x{:X}",
				sequence_number, reinterpret_cast<uintptr_t>(pCmd));
			FW_TRACE(message.c_str());
		}
		return;
	}

	bool bFallbackSendPacket = true;
	bool* pSendPacket = reinterpret_cast<bool*>(uintptr_t(_AddressOfReturnAddress()) + 0x20);
	const bool bHasRealSendPacket = Minidump::IsReadableAddress(reinterpret_cast<uintptr_t>(pSendPacket), sizeof(bool));
	if (!bHasRealSendPacket)
	{
		FDBG_ERROR("CreateMove.pSendPacket", "send packet pointer unreadable");
		FW_TRACE(("[ERROR] CreateMove pSendPacket pointer unreadable | address=" + Minidump::DescribeAddress(reinterpret_cast<uintptr_t>(pSendPacket))).c_str());
		pSendPacket = &bFallbackSendPacket;
	}

	static int nLoggedValidCmd = 0;
	if (nLoggedValidCmd++ < 5)
	{
		const auto message = std::format(
			"[CUSERCMD] BaseClientDLL_CreateMove: seq={} cmd={} tick={} buttons={} fwd={:.1f} side={:.1f} cmd_ptr=0x{:X} send_packet_ptr=0x{:X}",
			sequence_number, pCmd->command_number, pCmd->tick_count, pCmd->buttons, pCmd->forwardmove, pCmd->sidemove,
			reinterpret_cast<uintptr_t>(pCmd), reinterpret_cast<uintptr_t>(pSendPacket));
		FW_TRACE(message.c_str());
	}

	//	save old info
	static int nOldFlags = 0;
	static EHANDLE nOldGroundEnt = {};
	static Vec3 vOldAngles = pCmd->viewangles;
	static float fOldSide = pCmd->sidemove;
	static float fOldForward = pCmd->forwardmove;

	G::CurrentUserCmd = pCmd;

	if (!G::ShouldShift)
	{
		if (const auto& pLocal = g_EntityCache.GetLocal())
		{
			nOldFlags = pLocal->GetFlags();
			nOldGroundEnt = pLocal->m_hGroundEntity();

			if (const int MaxSpeed = pLocal->GetMaxSpeed()) {
				G::Frozen = MaxSpeed == 1;
			}

			// Update Global Info
			if (const auto& pWeapon = g_EntityCache.GetWeapon()) {
				const int nItemDefIndex = pWeapon->GetItemDefIndex();
				const int nOldItemDefIndex = G::CurItemDefIndex;

				G::CurItemDefIndex = nItemDefIndex;
				UpdateWeaponAttackState(pLocal, pWeapon, pCmd);

				if (nOldItemDefIndex != nItemDefIndex || !G::WeaponCanAttack) {
					G::WaitForShift = DT_WAIT_CALLS;
				}
			}

			if (!G::ShiftedTicks) {
				G::RechargeQueued = (Vars::Misc::CL_Move::RechargeWhileDead.Value && !pLocal->IsAlive()) || (Vars::Misc::CL_Move::AutoRecharge.Value && pLocal->GetVecVelocity().Length2D() < 5.0f && !(pCmd->buttons));
			}
		}
	}
	else if (const auto& pWeapon = g_EntityCache.GetWeapon())
	{
		if (const auto& pLocal = g_EntityCache.GetLocal())
		{
			UpdateWeaponAttackState(pLocal, pWeapon, pCmd);
		}
	}	//	we always need this :c 

	FDBG_CALL("CreateMove.Prediction.Update", I::Prediction->Update(I::ClientState->m_nDeltaTick,
		I::ClientState->m_nDeltaTick > 0,
		I::ClientState->last_command_ack,
		I::ClientState->lastoutgoingcommand + GetSafeCreateMoveChokedCommands()));

	// Run Features
	{
		FDBG_CALL("Feature.Misc.RunPre", F::Misc.RunPre(pCmd, pSendPacket));
		FDBG_CALL("Feature.Fedworking.Run", F::Fedworking.Run());
		FDBG_CALL("Feature.CameraWindow.Update", F::CameraWindow.Update());
		FDBG_CALL("Feature.BadActors.OnTick", F::BadActors.OnTick());
		FDBG_CALL("Feature.Backtrack.Run", F::Backtrack.Run(pCmd));

		FDBG_CALL("Feature.Ticks.StartPrediction", F::Ticks.StartPrediction(pCmd));
		{
			if (pCmd) {
				FDBG_CALL("Feature.Aimbot.Run", F::Aimbot.Run(pCmd));
				FDBG_CALL("Feature.Auto.Run", F::Auto.Run(pCmd));
				FDBG_CALL("Feature.Misc.RunMid", F::Misc.RunMid(pCmd, nOldGroundEnt));
			}
			else
			{
				FDBG_SKIP("CreateMove.PredictedFeatures", "pCmd null");
			}
		}
		FDBG_CALL("Feature.Ticks.EndPrediction", F::Ticks.EndPrediction(pCmd));

		if (bHasRealSendPacket)
		{
			FDBG_CALL("Feature.PacketManip.CreateMove", F::PacketManip.CreateMove(pCmd, pSendPacket, nOldGroundEnt, nOldFlags));
			FDBG_CALL("Feature.Ticks.CreateMove", F::Ticks.CreateMove(pCmd, pSendPacket));
		}
		else
		{
			FDBG_SKIP("Feature.PacketManip.CreateMove", "real send_packet pointer unavailable");
			FDBG_SKIP("Feature.Ticks.CreateMove", "real send_packet pointer unavailable");
			G::ShouldShift = false;
		}
		FDBG_CALL("Feature.CritHack.Run", F::CritHack.Run(pCmd));
		FDBG_CALL("Feature.Misc.RunPost", F::Misc.RunPost(pCmd, pSendPacket));
		FDBG_CALL("Feature.Resolver.CreateMove", F::Resolver.CreateMove());
		FDBG_CALL("Feature.Followbot.Run", F::Followbot.Run(pCmd));
	}

	if (bHasRealSendPacket && *pSendPacket)
	{
		FDBG_CALL("Feature.FakeAng.Run", F::FakeAng.Run(pCmd));
		F::FakeAng.DrawChams = Vars::AntiHack::AntiAim::Active.Value || Vars::Misc::CL_Move::Fakelag.Value;
	}
	else
	{
		FDBG_SKIP("Feature.FakeAng.Run", bHasRealSendPacket ? "send_packet false" : "real send_packet pointer unavailable");
		if (!bHasRealSendPacket)
		{
			F::FakeAng.DrawChams = false;
		}
	}

	G::ViewAngles = pCmd->viewangles;

	// Party Crasher: Crashes the party by spamming messages
	if (Vars::Misc::PartyCrasher.Value && !G::ShouldShift)
	{
		I::EngineClient->ClientCmd_Unrestricted("tf_party_chat \"FED@MA==\"");
	}

	// do this at the end just in case aimbot / triggerbot fired.
	if (const auto& pWeapon = g_EntityCache.GetWeapon(); const auto & pLocal = g_EntityCache.GetLocal())
	{
		if (pCmd->buttons & IN_ATTACK && (Vars::Misc::CL_Move::SafeTick.Value || (Vars::Misc::CL_Move::SafeTickAirOverride.Value && !pLocal->OnSolid())))
		{
			if (G::NextSafeTick > I::GlobalVars->tickcount && G::ShouldShift && G::ShiftedTicks)
			{
				pCmd->buttons &= ~IN_ATTACK;
			}
			else
			{
				G::NextSafeTick = I::GlobalVars->tickcount + g_ConVars.sv_maxusrcmdprocessticks_holdaim->GetInt() + 1;
			}
		}
	}

	FDBG_CALL("Feature.NoSpread.CreateMoveProjectile", F::NoSpread.CreateMoveProjectile(pCmd));
	FDBG_CALL("Feature.NoSpread.CreateMoveHitscan", F::NoSpread.CreateMoveHitscan(pCmd));
	if (bHasRealSendPacket)
	{
		FDBG_CALL("Feature.Ticks.FinalizeAntiWarp", F::Ticks.FinalizeAntiWarp(pCmd));
	}
	else
	{
		FDBG_SKIP("Feature.Ticks.FinalizeAntiWarp", "real send_packet pointer unavailable");
	}

	G::DebugSendPacket = bHasRealSendPacket ? *pSendPacket : true;
	G::DebugAttackButton = pCmd ? (pCmd->buttons & IN_ATTACK) != 0 : false;
	if (const auto& pLocal = g_EntityCache.GetLocal())
	{
		G::DebugCurTime = static_cast<float>(pLocal->GetTickBase()) * I::GlobalVars->interval_per_tick;
		G::DebugNextAttack = pLocal->GetNextAttack();
	}
	else
	{
		G::DebugCurTime = 0.f;
		G::DebugNextAttack = 0.f;
	}
	if (const auto& pWeapon = g_EntityCache.GetWeapon())
	{
		G::DebugNextPrimaryAttack = pWeapon->GetNextPrimaryAttack();
		G::DebugNextSecondaryAttack = pWeapon->GetNextSecondaryAttack();
	}
	else
	{
		G::DebugNextPrimaryAttack = 0.f;
		G::DebugNextSecondaryAttack = 0.f;
	}

	G::LastUserCmd = pCmd;

	bool bTauntControl = false;
	FDBG_CALL("Feature.Misc.TauntControl", bTauntControl = F::Misc.TauntControl(pCmd));
	const bool bShouldSkip = (G::SilentTime || G::AAActive || G::HitscanSilentActive || G::AvoidingBackstab || !G::UpdateView || !bTauntControl);
	if (!bShouldSkip)
	{
		FDBG_CALL("CreateMove.Prediction.SetLocalViewAngles", I::Prediction->SetLocalViewAngles(pCmd->viewangles));
	}
	else
	{
		FDBG_SKIP("CreateMove.Prediction.SetLocalViewAngles", std::format("silent={} aa={} hitscanSilent={} backstab={} updateView={} tauntControl={}",
			G::SilentTime, G::AAActive, G::HitscanSilentActive, G::AvoidingBackstab, G::UpdateView, bTauntControl));
	}

	FDBG_CALL("CreateMove.UpdateVerifiedUserCmd", UpdateVerifiedUserCmd(sequence_number, pCmd));
}
