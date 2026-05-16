#include "TickHandler.h"
#include "../Prediction/Prediction.h"
#include "../../Utils/FunctionalDebug/FunctionalDebug.h"

static Vec3 s_vAntiWarpVelocity = {};
static int s_iAntiWarpMaxTicks = 0;

namespace
{
	int GetSafeChokedCommands()
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

	bool TickTraceEnabled()
	{
		return Vars::Debug::TickbaseLogging.Value;
	}

	void TickTraceEvery(const int slot, const uint64_t throttleMs, const std::string& message)
	{
		if (!TickTraceEnabled())
		{
			return;
		}

		static uint64_t s_LastTraceMs[16] = {};
		const int safeSlot = std::clamp(slot, 0, 15);
		const uint64_t now = GetTickCount64();
		if (now - s_LastTraceMs[safeSlot] < throttleMs)
		{
			return;
		}

		s_LastTraceMs[safeSlot] = now;
		FW_TRACE(message.c_str());
	}

	void TickTraceEvent(const std::string& message)
	{
		if (TickTraceEnabled())
		{
			FW_TRACE(message.c_str());
		}
	}

	Vec3 GetAntiWarpVelocity(CBaseEntity* pLocal, const CUserCmd* pCmd)
	{
		Vec3 vVelocity = pLocal ? pLocal->m_vecVelocity() : Vec3();
		vVelocity.z = 0.f;
		if (vVelocity.Length2D() > 1.f || !pCmd)
		{
			return vVelocity;
		}

		Vec3 vForward = {}, vRight = {};
		Math::AngleVectors({ 0.f, pCmd->viewangles.y, 0.f }, &vForward, &vRight, nullptr);

		vVelocity = vForward * pCmd->forwardmove + vRight * pCmd->sidemove;
		vVelocity.z = 0.f;
		return vVelocity;
	}

	int GetAntiWarpTicks(const int iTicks)
	{
		const int iWishTicks = std::max(Vars::Misc::CL_Move::DTTicks.Value, 1);
		const int iExecutableTicks = std::min(iWishTicks, MAX_NEW_COMMANDS);
		return std::clamp(iTicks, 0, iExecutableTicks);
	}
}

void CTickshiftHandler::UpdateTickRate()
{
	if (!I::GlobalVars || I::GlobalVars->interval_per_tick <= 0.f)
	{
		return;
	}

	const int iCurrentTickRate = std::max(1, static_cast<int>(std::round(1.f / I::GlobalVars->interval_per_tick)));
	const int iCurrentMaxProcessTicks = g_ConVars.sv_maxusrcmdprocessticks ? std::clamp(g_ConVars.sv_maxusrcmdprocessticks->GetInt(), 1, 24) : 24;
	if (iTickRate != iCurrentTickRate)
	{
		iTickRate = iCurrentTickRate;
		FW_TRACE(std::format("[CL_MOVE] tickrate updated tickrate={} interval={:.6f} sv_maxusrcmdprocessticks={} max_shift={}",
			iTickRate, I::GlobalVars->interval_per_tick, iCurrentMaxProcessTicks, std::clamp(iCurrentMaxProcessTicks - 3, 1, 21)).c_str());
	}

	static int iLastMaxProcessTicks = -1;
	if (iLastMaxProcessTicks != iCurrentMaxProcessTicks)
	{
		iLastMaxProcessTicks = iCurrentMaxProcessTicks;
		TickTraceEvent(std::format("[TICKDBG] server cmd budget sv_maxusrcmdprocessticks={} max_shift={} tickrate={} interval={:.6f}",
			iCurrentMaxProcessTicks, std::clamp(iCurrentMaxProcessTicks - 3, 1, 21), iTickRate, I::GlobalVars->interval_per_tick));
	}

	iMaxUsrCmdProcessTicks = iCurrentMaxProcessTicks;
	iMaxShift = std::clamp(iMaxUsrCmdProcessTicks - 3, 1, 21);
}

void CTickshiftHandler::Speedhack(CUserCmd* pCmd)
{
	bSpeedhack = Vars::Misc::CL_Move::SEnabled.Value;
	if (!bSpeedhack)
	{
		return;
	}

	bTeleport = false;
	bRecharge = false;
	bDoubletap = false;
	bAntiWarp = false;
}

void CTickshiftHandler::Recharge(CUserCmd* pCmd)
{
	static KeyHelper kRecharge{ &Vars::Misc::CL_Move::RechargeKey.Value };
	const bool bManualRecharge = kRecharge.Down();
	const bool bHasMovementInput = pCmd && (fabsf(pCmd->forwardmove) > 0.01f || fabsf(pCmd->sidemove) > 0.01f || fabsf(pCmd->upmove) > 0.01f
		|| (pCmd->buttons & (IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT | IN_JUMP | IN_DUCK)));

	if (bHasMovementInput && !bManualRecharge)
	{
		bRecharge = false;
		G::RechargeQueued = false;
	}

	const bool bBusy = bTeleport || bDoubletap || bSpeedhack;
	const bool bRequested = bManualRecharge || G::RechargeQueued;
	const bool bTeleportCompliant = Vars::Misc::CL_Move::TeleportMode.Value != 2 ? iAvailableTicks < Vars::Misc::CL_Move::DTTicks.Value : !iAvailableTicks;
	bRecharge = (!bBusy && bTeleportCompliant && bRequested) || bRecharge;
	if (bRecharge)
	{
		iShiftedGoal = std::min(iAvailableTicks + 1, iMaxShift);
	}
}

void CTickshiftHandler::Teleport(CUserCmd* pCmd)
{
	static KeyHelper kTeleport{ &Vars::Misc::CL_Move::TeleportKey.Value };
	bTeleport = (((!bRecharge && !bDoubletap && !bSpeedhack) && kTeleport.Down()) || (bTeleport && Vars::Misc::CL_Move::TeleportMode.Value != 2)) && iAvailableTicks;
	if (bTeleport)
	{
		const int iWishTicks = Vars::Misc::CL_Move::TeleportMode.Value > 0 && iAvailableTicks > 2
			? std::clamp(Vars::Misc::CL_Move::TeleportFactor.Value, 2, iAvailableTicks)
			: iAvailableTicks;
		iShiftedGoal = std::max(iAvailableTicks - iWishTicks, 0);
	}
}

int CTickshiftHandler::GetShotsWithinPacket(CBaseCombatWeapon* pWeapon, int iTicks)
{
	if (!pWeapon || iTicks <= 0 || TICK_INTERVAL <= 0.f)
	{
		return 1;
	}

	iTicks = std::min(22, iTicks);

	int iDelay = 1;
	switch (pWeapon->GetWeaponID())
	{
		case TF_WEAPON_MINIGUN:
		case TF_WEAPON_PIPEBOMBLAUNCHER:
		case TF_WEAPON_CANNON:
			iDelay = 2;
			break;
	}

	const int iFireRateTicks = std::max(1, static_cast<int>(std::ceil(pWeapon->GetFireRate() / TICK_INTERVAL)));
	return 1 + std::max(iTicks - iDelay, 0) / iFireRateTicks;
}

bool CTickshiftHandler::CanDoubletap(CBaseCombatWeapon* pWeapon)
{
	if (!pWeapon || Vars::Misc::CL_Move::DTMode.Value == 3 || bTeleport || bRecharge || bSpeedhack)
	{
		return false;
	}

	if (G::WaitForShift && Vars::Misc::CL_Move::WaitForDT.Value)
	{
		return false;
	}

	switch (pWeapon->GetWeaponID())
	{
		case TF_WEAPON_PDA:
		case TF_WEAPON_PDA_ENGINEER_BUILD:
		case TF_WEAPON_PDA_ENGINEER_DESTROY:
		case TF_WEAPON_PDA_SPY:
		case TF_WEAPON_PDA_SPY_BUILD:
		case TF_WEAPON_BUILDER:
		case TF_WEAPON_INVIS:
		case TF_WEAPON_GRAPPLINGHOOK:
		case TF_WEAPON_JAR_MILK:
		case TF_WEAPON_LUNCHBOX:
		case TF_WEAPON_BUFF_ITEM:
		case TF_WEAPON_ROCKETPACK:
		case TF_WEAPON_JAR_GAS:
		case TF_WEAPON_LASER_POINTER:
		case TF_WEAPON_MEDIGUN:
		case TF_WEAPON_SNIPERRIFLE:
		case TF_WEAPON_SNIPERRIFLE_DECAP:
		case TF_WEAPON_SNIPERRIFLE_CLASSIC:
		case TF_WEAPON_COMPOUND_BOW:
		case TF_WEAPON_JAR:
			return false;
		default:
			break;
	}

	const int iShiftedTicks = std::max(G::ShiftedTicks, 0);
	const int iWishTicks = std::max(Vars::Misc::CL_Move::DTTicks.Value, 1);
	return iShiftedTicks >= iWishTicks;
}

void CTickshiftHandler::Doubletap(const CUserCmd* pCmd, CBaseEntity* pLocal)
{
	static KeyHelper kDoubletap{ &Vars::Misc::CL_Move::DoubletapKey.Value };
	if (bTeleport || bRecharge || bSpeedhack)
	{
		return;
	}

	if (G::WaitForShift && Vars::Misc::CL_Move::WaitForDT.Value)
	{
		return;
	}

	if (G::ShouldShift || !pCmd || !G::ShiftedTicks)
	{
		return;
	}

	if (G::ShiftedTicks < std::max(Vars::Misc::CL_Move::DTTicks.Value, 1))
	{
		return;
	}

	switch (Vars::Misc::CL_Move::DTMode.Value)
	{
		case 0:
		{
			if (!kDoubletap.Down())
			{
				return;
			}
			break;
		}
		case 2:
		{
			if (kDoubletap.Down())
			{
				return;
			}
			break;
		}
		case 3:
		{
			return;
		}
		default:
			break;
	}

	CBaseCombatWeapon* pWeapon = g_EntityCache.GetWeapon();
	if (!CanDoubletap(pWeapon))
	{
		return;
	}

	const bool bAttacking = G::CurWeaponType == EWeaponType::MELEE
		? (pCmd->buttons & IN_ATTACK) && G::WeaponCanAttack
		: F::AimbotGlobal.IsAttacking();

	if (!bAttacking || (!G::WeaponCanAttack && !(pWeapon && pWeapon->IsInReload())))
	{
		return;
	}

	bDoubletap = G::ShouldShift = Vars::Misc::CL_Move::NotInAir.Value ? pLocal->OnSolid() : true;
	if (bDoubletap && Vars::Misc::CL_Move::AntiWarp.Value)
	{
		bAntiWarp = pLocal->OnSolid();
	}
	if (bDoubletap)
	{
		iShiftedGoal = std::max(iAvailableTicks - std::max(Vars::Misc::CL_Move::DTTicks.Value, 1) + 1, 0);
	}
}

void CTickshiftHandler::AntiWarp(CBaseEntity* pLocal, float flYaw, float& flForwardMove, float& flSideMove, int iTicks)
{
	if (!pLocal || iTicks <= 0)
	{
		return;
	}

	const float flSpeed = s_vAntiWarpVelocity.Length2D();
	if (flSpeed <= 1.f)
	{
		return;
	}

	s_iAntiWarpMaxTicks = std::max(iTicks + 1, s_iAntiWarpMaxTicks);

	Vec3 vAngles = {};
	Math::VectorAngles(s_vAntiWarpVelocity, vAngles);
	vAngles.y = flYaw - vAngles.y;

	Vec3 vForward = {};
	Math::AngleVectors(vAngles, &vForward);
	vForward *= flSpeed;

	if (iTicks > std::max(s_iAntiWarpMaxTicks - 8, 3))
	{
		flForwardMove = -vForward.x;
		flSideMove = -vForward.y;
	}
	else if (iTicks > 3)
	{
		flForwardMove = 0.f;
		flSideMove = 0.f;
	}
	else
	{
		flForwardMove = vForward.x;
		flSideMove = vForward.y;
	}
}

void CTickshiftHandler::AntiWarp(CBaseEntity* pLocal, CUserCmd* pCmd)
{
	if (!pLocal || !pCmd)
	{
		bAntiWarp = false;
		s_iAntiWarpMaxTicks = 0;
		return;
	}

	if (bAntiWarp && Vars::Misc::CL_Move::AntiWarp.Value)
	{
		const int iAntiWarpTicks = GetAntiWarpTicks(G::ShiftedTicks);
		const float flOldForward = pCmd->forwardmove;
		const float flOldSide = pCmd->sidemove;
		AntiWarp(pLocal, pCmd->viewangles.y, pCmd->forwardmove, pCmd->sidemove, iAntiWarpTicks);
		TickTraceEvery(3, 100, std::format("[TICKDBG] antiwarp ticks={} raw={} shifted={} vel2d={:.1f} yaw={:.1f} move={:.1f}/{:.1f}->{:.1f}/{:.1f}",
			iAntiWarpTicks, std::max(Vars::Misc::CL_Move::DTTicks.Value, 1), G::ShiftedTicks, s_vAntiWarpVelocity.Length2D(), pCmd->viewangles.y,
			flOldForward, flOldSide, pCmd->forwardmove, pCmd->sidemove));
		return;
	}

	s_vAntiWarpVelocity = GetAntiWarpVelocity(pLocal, pCmd);
	s_iAntiWarpMaxTicks = 0;
}

void CTickshiftHandler::ManagePacket(bool* pSendPacket)
{
	if (!pSendPacket)
	{
		return;
	}

	if (bDoubletap || bTeleport || bSpeedhack)
	{
		if ((bTeleport || bSpeedhack) && F::AimbotGlobal.IsAttacking())
		{
			*pSendPacket = true;
			return;
		}

		*pSendPacket = iShiftedGoal == iAvailableTicks;
		if (GetSafeChokedCommands() >= 21)
		{
			*pSendPacket = true;
		}

		TickTraceEvery(4, 100, std::format("[TICKDBG] packet send={} shifted={} choked={} flags(dt={} tp={} rc={} aw={} sp={}) attacking={} deficit={}",
			*pSendPacket,
			G::ShiftedTicks,
			GetSafeChokedCommands(),
			bDoubletap, bTeleport, bRecharge, bAntiWarp, bSpeedhack,
			F::AimbotGlobal.IsAttacking(),
			iDeficit));
	}
}

void CTickshiftHandler::StartPrediction(CUserCmd* pCmd)
{
	FDBG_SCOPE("Feature.Ticks.StartPrediction");

	CBaseEntity* pLocal = g_EntityCache.GetLocal();
	if (!pCmd)
	{
		FDBG_SKIP("Feature.Ticks.StartPrediction", "pCmd null");
		return;
	}

	if (!pLocal || !pLocal->IsAlive())
	{
		FDBG_CALL("Feature.EnginePrediction.Start", F::EnginePrediction.Start(pCmd));
		return;
	}

	Vec2 vOriginalMove = {};
	int iOriginalButtons = 0;
	bPredictAntiWarp = false;

	if (Vars::Misc::CL_Move::Enabled.Value && Vars::Misc::CL_Move::AntiWarp.Value && pLocal->OnSolid())
	{
		int iTicks = 0;
		if (bAntiWarp)
		{
			iTicks = GetAntiWarpTicks(G::ShiftedTicks);
		}
		else if (Vars::Misc::CL_Move::Doubletap.Value && Vars::Misc::CL_Move::DTMode.Value != 3
			&& !G::WaitForShift && !bDoubletap && !bTeleport && !bRecharge && !bSpeedhack
			&& CanDoubletap(g_EntityCache.GetWeapon()))
		{
			iTicks = GetAntiWarpTicks(std::max(Vars::Misc::CL_Move::DTTicks.Value, 1));
		}

		if (iTicks > 0)
		{
			bPredictAntiWarp = true;
			vOriginalMove = { pCmd->forwardmove, pCmd->sidemove };
			iOriginalButtons = pCmd->buttons;
			AntiWarp(pLocal, pCmd->viewangles.y, pCmd->forwardmove, pCmd->sidemove, iTicks);
			TickTraceEvery(5, 250, std::format("[TICKDBG] predict antiwarp ticks={} shifted={} antiwarp={} cmd={} buttons=0x{:X} move={:.1f}/{:.1f}",
				iTicks, G::ShiftedTicks, bAntiWarp, pCmd->command_number, iOriginalButtons, vOriginalMove.x, vOriginalMove.y));
		}
	}

	FDBG_CALL("Feature.EnginePrediction.Start", F::EnginePrediction.Start(pCmd));

	if (bPredictAntiWarp)
	{
		pCmd->forwardmove = vOriginalMove.x;
		pCmd->sidemove = vOriginalMove.y;
		pCmd->buttons = iOriginalButtons;
	}
}

void CTickshiftHandler::EndPrediction(CUserCmd* pCmd)
{
	FDBG_SCOPE("Feature.Ticks.EndPrediction");

	if (bPredictAntiWarp && !bAntiWarp && !F::AimbotGlobal.IsAttacking())
	{
		FDBG_CALL("Feature.EnginePrediction.End.AntiWarpRestart", F::EnginePrediction.End(pCmd));
		FDBG_CALL("Feature.EnginePrediction.Start.AntiWarpRestart", F::EnginePrediction.Start(pCmd));
	}

	FDBG_CALL("Feature.EnginePrediction.End", F::EnginePrediction.End(pCmd));
	bPredictAntiWarp = false;
}

bool CTickshiftHandler::MeleeDoubletapCheck(CBaseEntity* pLocal)
{
	if (!pLocal || bTeleport || bRecharge || bSpeedhack)
	{
		return false;
	}

	static KeyHelper kDoubletap{ &Vars::Misc::CL_Move::DoubletapKey.Value };
	if (G::WaitForShift && Vars::Misc::CL_Move::WaitForDT.Value)
	{
		return false;
	}

	switch (Vars::Misc::CL_Move::DTMode.Value)
	{
		case 0:
		{
			if (!kDoubletap.Down())
			{
				return false;
			}
			break;
		}
		case 2:
		{
			if (kDoubletap.Down())
			{
				return false;
			}
			break;
		}
		case 3:
		{
			return false;
		}
		default:
			break;
	}

	if (!CanDoubletap(g_EntityCache.GetWeapon()))
	{
		return false;
	}

	return Vars::Misc::CL_Move::NotInAir.Value ? pLocal->OnSolid() : true;
}

void CTickshiftHandler::CLMoveFunc(float accumulated_extra_samples, bool bFinalTick)
{
	FDBG_SCOPE("Feature.Ticks.CLMoveFunc");

	static auto CL_Move = g_HookManager.GetMapHooks()["CL_Move"];
	if (!CL_Move)
	{
		CL_Move = g_HookManager.GetMapHooks()["CL_Move"];
		FDBG_ERROR("Feature.Ticks.CLMoveFunc", "CL_Move hook object missing");
		return;
	}

	if (iAvailableTicks <= 0)
	{
		iAvailableTicks = 0;
		G::ShiftedTicks = 0;
		G::WaitForShift = std::clamp(G::WaitForShift - 1, 0, 26);
		return CL_Move->Original<void(__cdecl*)(float, bool)>()(accumulated_extra_samples, bFinalTick);
	}

	iAvailableTicks--;
	G::ShiftedTicks = iAvailableTicks;
	G::WaitForShift = std::clamp(G::WaitForShift - 1, 0, 26);
	const bool bTickbaseActive = bDoubletap || bTeleport || bRecharge || bSpeedhack;
	bGoalReached = bFinalTick && (!bTickbaseActive || iAvailableTicks == iShiftedGoal);

	return CL_Move->Original<void(__cdecl*)(float, bool)>()(accumulated_extra_samples, bFinalTick);
}

void CTickshiftHandler::CLMove(float accumulated_extra_samples, bool bFinalTick)
{
	FDBG_SCOPE_SLOW("Feature.Ticks.CLMoveBody", FunctionalDebug::HOOK_SLOW_US);
	UpdateTickRate();

	FDBG_CALL("Feature.NetworkFix.FixInputDelay", F::NetworkFix.FixInputDelay(bFinalTick));

	bIgnoreSendNetMsg = false;
	iAvailableTicks = std::max(G::ShiftedTicks, 0);
	while (iAvailableTicks > iMaxShift)
	{
		TickTraceEvery(7, 100, std::format("[TICKDBG] clmove trim avail={} maxshift={} shifted={} deficit={} pred={}",
			iAvailableTicks, iMaxShift, G::ShiftedTicks, iDeficit, iPredicted));
		CLMoveFunc(accumulated_extra_samples, false);
	}

	iAvailableTicks++;
	iPredicted++;
	G::ShiftedTicks = iAvailableTicks;
	iShiftedGoal = std::clamp(iShiftedGoal, 0, iMaxShift);

	TickTraceEvery(0, 250, std::format("[TICKDBG] clmove final={} avail={} goal={} start={} shifted={} shifting={} choked={} wait={} deficit={} pred={} flags(dt={} tp={} rc={} aw={} sp={})",
		bFinalTick,
		iAvailableTicks,
		iShiftedGoal,
		iShiftStart,
		G::ShiftedTicks,
		bShifting,
		GetSafeChokedCommands(),
		G::WaitForShift,
		iDeficit,
		iPredicted,
		bDoubletap, bTeleport, bRecharge, bAntiWarp, bSpeedhack));

	TickTraceEvery(6, 500, std::format("[TICKDBG] clmove budget tickrate={} interval={:.6f} maxprocess={} maxshift={} final={} extra={:.6f} shifted={} avail={} goal={} deficit={} wait={}",
		iTickRate,
		I::GlobalVars ? I::GlobalVars->interval_per_tick : 0.f,
		iMaxUsrCmdProcessTicks,
		iMaxShift,
		bFinalTick,
		accumulated_extra_samples,
		G::ShiftedTicks,
		iAvailableTicks,
		iShiftedGoal,
		iDeficit,
		G::WaitForShift));

	if (!Vars::Misc::CL_Move::Enabled.Value || I::EngineClient->IsPlayingTimeDemo())
	{
		G::WaitForShift = G::ShiftedTicks = 0;
		while (iAvailableTicks > 1)
		{
			CLMoveFunc(accumulated_extra_samples, false);
		}
		return CLMoveFunc(accumulated_extra_samples, true);
	}

	auto runTeleportShift = [&]()
	{
		const int iStartTicks = iAvailableTicks;
		iShiftedGoal = std::clamp(iShiftedGoal, 0, iMaxShift);
		iShiftStart = iAvailableTicks - 1;
		bShifted = false;

		TickTraceEvent(std::format("[TICKDBG] shift teleport start start={} goal={} avail={} shifted={} choked={} mode={} factor={}",
			iStartTicks, iShiftedGoal, iAvailableTicks, G::ShiftedTicks,
			GetSafeChokedCommands(),
			Vars::Misc::CL_Move::TeleportMode.Value,
			Vars::Misc::CL_Move::TeleportFactor.Value));

		while (iAvailableTicks > iShiftedGoal)
		{
			bShifting = bShifted |= iAvailableTicks - 1 != iShiftedGoal;
			CLMoveFunc(accumulated_extra_samples, iAvailableTicks - 1 == iShiftedGoal);
		}

		bShifting = false;
		bTeleport = false;
		bAntiWarp = false;
		G::Teleporting = false;
		TickTraceEvent(std::format("[TICKDBG] shift teleport end avail={} shifted={} choked={}",
			iAvailableTicks, G::ShiftedTicks, GetSafeChokedCommands()));
	};

	auto runDoubletapShift = [&]()
	{
		const int iStartTicks = iAvailableTicks;
		iShiftedGoal = std::clamp(iShiftedGoal, 0, iMaxShift);
		iShiftStart = iAvailableTicks - 1;
		bShifted = false;
		TickTraceEvent(std::format("[TICKDBG] shift doubletap start ticks={} goal={} shifted={} choked={} antiwarp={} attacking={} weaponCan={}",
			iStartTicks, iShiftedGoal, G::ShiftedTicks,
			GetSafeChokedCommands(),
			bAntiWarp, F::AimbotGlobal.IsAttacking(), G::WeaponCanAttack));

		while (iAvailableTicks > iShiftedGoal)
		{
			bShifting = bShifted |= iAvailableTicks - 1 != iShiftedGoal;
			CLMoveFunc(accumulated_extra_samples, iAvailableTicks - 1 == iShiftedGoal);
		}

		bShifting = false;
		bDoubletap = false;
		bAntiWarp = false;
		G::ShouldShift = false;
		TickTraceEvent(std::format("[TICKDBG] shift doubletap end avail={} shifted={} choked={}",
			iAvailableTicks, G::ShiftedTicks, GetSafeChokedCommands()));
	};

	if (bTeleport && iAvailableTicks > 0)
	{
		runTeleportShift();
		return;
	}

	if (iDeficit && iAvailableTicks < 22 && Vars::Misc::CL_Move::AutoRetain.Value && G::ShiftedTicks > 1)
	{
		iDeficit--;
		return;
	}
	else if (iDeficit)
	{
		iDeficit = 0;
	}

	if (bRecharge)
	{
		if (iAvailableTicks <= Vars::Misc::CL_Move::DTTicks.Value)
		{
			static int nLoggedRecharge = 0;
			if (nLoggedRecharge++ < 5)
			{
				FW_TRACE(std::format("[CL_MOVE] recharge hold shifted={} dt_ticks={} tickrate={}",
					G::ShiftedTicks, Vars::Misc::CL_Move::DTTicks.Value, iTickRate).c_str());
			}
			return;
		}

		bRecharge = false;
		G::RechargeQueued = false;
		G::WaitForShift = std::max(iTickRate - Vars::Misc::CL_Move::DTTicks.Value, 0);
	}

	if (bDoubletap && iAvailableTicks > 0)
	{
		runDoubletapShift();
		return;
	}

	if (bSpeedhack)
	{
		iAvailableTicks = 0;
		iShiftedGoal = 0;
		const int iFactor = std::max(Vars::Misc::CL_Move::SFactor.Value, 1);
		for (int i = 0; i < iFactor; i++)
		{
			CLMoveFunc(accumulated_extra_samples, i == iFactor - 1);
		}
		return;
	}

	if (I::GlobalVars && I::GlobalVars->tickcount >= iNextPassiveTick && Vars::Misc::CL_Move::PassiveRecharge.Value && iTickRate > 0 && G::ShiftedTicks <= Vars::Misc::CL_Move::DTTicks.Value)
	{
		const int iPassiveDelay = std::max(1, iTickRate / std::max(1, Vars::Misc::CL_Move::PassiveRecharge.Value));
		iNextPassiveTick = I::GlobalVars->tickcount + iPassiveDelay;
		static int nLoggedPassive = 0;
		if (nLoggedPassive++ < 5)
		{
			FW_TRACE(std::format("[CL_MOVE] passive recharge hold shifted={} tickrate={} delay={}",
				G::ShiftedTicks, iTickRate, iPassiveDelay).c_str());
		}
		return;
	}

	G::ShouldShift = false;
	CLMoveFunc(accumulated_extra_samples, true);

	if (bTeleport && iAvailableTicks > 0)
	{
		runTeleportShift();
		return;
	}

	if (bDoubletap && iAvailableTicks > 0)
	{
		runDoubletapShift();
		return;
	}
}

void CTickshiftHandler::CreateMove(CUserCmd* pCmd, bool* pSendPacket)
{
	FDBG_SCOPE("Feature.Ticks.CreateMoveBody");
	UpdateTickRate();

	CBaseEntity* pLocal = g_EntityCache.GetLocal();
	if (!pLocal || !pLocal->IsAlive())
	{
		FDBG_SKIP("Feature.Ticks.CreateMoveBody", "local null or dead");
		return;
	}

	if (!Vars::Misc::CL_Move::Enabled.Value)
	{
		FDBG_SKIP("Feature.Ticks.CreateMoveBody", "CL_Move disabled");
		return;
	}

	if (bGoalReached)
	{
		FDBG_CALL("Feature.Ticks.Recharge", Recharge(pCmd));
		FDBG_CALL("Feature.Ticks.Teleport", Teleport(pCmd));
		FDBG_CALL("Feature.Ticks.Doubletap", Doubletap(pCmd, pLocal));
		FDBG_CALL("Feature.Ticks.Speedhack", Speedhack(pCmd));
	}
	else
	{
		FDBG_SKIP("Feature.Ticks.Recharge", "shift goal not reached");
		FDBG_SKIP("Feature.Ticks.Teleport", "shift goal not reached");
		FDBG_SKIP("Feature.Ticks.Doubletap", "shift goal not reached");
		FDBG_SKIP("Feature.Ticks.Speedhack", "shift goal not reached");
	}
	FDBG_CALL("Feature.Ticks.AntiWarp", AntiWarp(pLocal, pCmd));

	G::ShouldShift = bDoubletap;
	G::Teleporting = bTeleport;
	G::Recharging = bRecharge;

	FDBG_CALL("Feature.Ticks.ManagePacket", ManagePacket(pSendPacket));

	const int iStateMask = (bDoubletap ? 1 : 0) | (bTeleport ? 2 : 0) | (bRecharge ? 4 : 0) | (bAntiWarp ? 8 : 0) | (bSpeedhack ? 16 : 0);
	static int iLastStateMask = -1;
	if (iStateMask != iLastStateMask)
	{
		TickTraceEvent(std::format("[TICKDBG] state dt={} tp={} rc={} aw={} sp={} shifted={} avail={} wait={} choked={} send={}",
			bDoubletap, bTeleport, bRecharge, bAntiWarp, bSpeedhack,
			G::ShiftedTicks, iAvailableTicks, G::WaitForShift,
			GetSafeChokedCommands(),
			pSendPacket ? *pSendPacket : false));
		iLastStateMask = iStateMask;
	}

	TickTraceEvery(1, 250, std::format("[TICKDBG] cm cmd={} tick={} buttons=0x{:X} attack={} fwd={:.1f} side={:.1f} send={} shifted={} avail={} wait={} choked={} flags(dt={} tp={} rc={} aw={} sp={}) weaponCan={} attacking={} deficit={} pred={}",
		pCmd ? pCmd->command_number : -1,
		pCmd ? pCmd->tick_count : -1,
		pCmd ? pCmd->buttons : 0,
		pCmd ? (pCmd->buttons & IN_ATTACK) != 0 : false,
		pCmd ? pCmd->forwardmove : 0.f,
		pCmd ? pCmd->sidemove : 0.f,
		pSendPacket ? *pSendPacket : false,
		G::ShiftedTicks,
		iAvailableTicks,
		G::WaitForShift,
		GetSafeChokedCommands(),
		bDoubletap, bTeleport, bRecharge, bAntiWarp, bSpeedhack,
		G::WeaponCanAttack,
		F::AimbotGlobal.IsAttacking(),
		iDeficit,
		iPredicted));
}

void CTickshiftHandler::FinalizeAntiWarp(CUserCmd* pCmd)
{
	if (!bAntiWarp || !Vars::Misc::CL_Move::AntiWarp.Value || !pCmd)
	{
		return;
	}

	CBaseEntity* pLocal = g_EntityCache.GetLocal();
	if (!pLocal || !pLocal->IsAlive())
	{
		return;
	}

	const float flOldForward = pCmd->forwardmove;
	const float flOldSide = pCmd->sidemove;
	const int iAntiWarpTicks = GetAntiWarpTicks(G::ShiftedTicks);
	AntiWarp(pLocal, pCmd->viewangles.y, pCmd->forwardmove, pCmd->sidemove, iAntiWarpTicks);

	TickTraceEvery(6, 100, std::format("[TICKDBG] antiwarp final ticks={} raw={} shifted={} vel2d={:.1f} yaw={:.1f} move={:.1f}/{:.1f}->{:.1f}/{:.1f}",
		iAntiWarpTicks, std::max(Vars::Misc::CL_Move::DTTicks.Value, 1), G::ShiftedTicks, s_vAntiWarpVelocity.Length2D(), pCmd->viewangles.y,
		flOldForward, flOldSide, pCmd->forwardmove, pCmd->sidemove));
}

void CTickshiftHandler::Reset()
{
	bSpeedhack = false;
	bDoubletap = false;
	bRecharge = false;
	bTeleport = false;
	bAntiWarp = false;
	bPredictAntiWarp = false;
	bGoalReached = true;
	bShifted = false;
	bShifting = false;
	bIgnoreSendNetMsg = false;

	iAvailableTicks = 0;
	iShiftedGoal = 0;
	iShiftStart = 0;
	iPredicted = 0;
	iNextPassiveTick = 0;
	iTickRate = 0;
	iDeficit = 0;

	G::ShiftedTicks = 0;
	G::ShouldShift = false;
	G::Teleporting = false;
	G::Recharging = false;
	G::WaitForShift = 0;
	G::RechargeQueued = false;

	UpdateTickRate();
}

void CTickshiftHandler::DrawDebug()
{
	int yoffset = 50, xoffset = 960;
	const auto& menuFont = g_Draw.GetFont(FONT_MENU);

	g_Draw.String(menuFont, xoffset, yoffset += 15, { 255, 255, 225, 255 }, ALIGN_CENTER, "TickShift Handler (DEBUG)");
	g_Draw.String(menuFont, xoffset, yoffset += 15, { 255, 255, 225, 255 }, ALIGN_CENTER, "Predicted Storage = %d", iPredicted);
	g_Draw.String(menuFont, xoffset, yoffset += 15, { 255, 255, 225, 255 }, ALIGN_CENTER, "Reported Storage = %d", iAvailableTicks);
	const int iDelta = fabs(iPredicted - iAvailableTicks);
	g_Draw.String(menuFont, xoffset, yoffset += 15, { 255, 255, 225, 255 }, ALIGN_CENTER, "Delta Storages = %d", iDelta);
}
