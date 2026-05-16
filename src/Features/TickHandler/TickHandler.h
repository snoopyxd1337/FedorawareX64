#pragma once
#include "../Feature.h"
#include "../../Hooks/HookManager.h"
#include "../../Hooks/Hooks.h"
#include "../Aimbot/AimbotGlobal/AimbotGlobal.h"
#include "NetworkFix/NetworkFix.h"

class CTickshiftHandler
{
	// logic
	void Speedhack(CUserCmd* pCmd);
	void Recharge(CUserCmd* pCmd);
	void Teleport(CUserCmd* pCmd);
	void Doubletap(const CUserCmd* pCmd, CBaseEntity* pLocal);
	bool CanDoubletap(CBaseCombatWeapon* pWeapon);
	int GetShotsWithinPacket(CBaseCombatWeapon* pWeapon, int iTicks);
	void AntiWarp(CBaseEntity* pLocal, float flYaw, float& flForwardMove, float& flSideMove, int iTicks);
	void AntiWarp(CBaseEntity* pLocal, CUserCmd* pCmd);
	void ManagePacket(bool* pSendPacket);
	void UpdateTickRate();

	// utils
	void CLMoveFunc(float accumulated_extra_samples, bool bFinalTick);

	//
	bool bSpeedhack = false;
	bool bTeleport = false;
	bool bRecharge = false;
	bool bDoubletap = false;
	bool bAntiWarp = false;
	bool bPredictAntiWarp = false;
	bool bGoalReached = true;
	bool bShifted = false;
	int iAvailableTicks = 0;	// should be equal to G::ShiftedTicks
	int iShiftedGoal = 0;
	int iShiftStart = 0;
	int iMaxUsrCmdProcessTicks = 24;
	int iMaxShift = 21;
	int iNextPassiveTick = 0;
	int iTickRate = 0;

public:
	bool bIgnoreSendNetMsg = false;
	bool bShifting = false;
	int iDeficit = 0;
	int iPredicted = 0;	// DEBUG

	int GetShiftedTicks() const { return iAvailableTicks; }
	int GetShiftedGoal() const { return iShiftedGoal; }
	int GetShiftStart() const { return iShiftStart; }
	int GetTickRate() const { return iTickRate; }
	int GetMaxUserCmdProcessTicks() const { return iMaxUsrCmdProcessTicks; }
	int GetMaxShift() const { return iMaxShift; }
	bool IsTickbaseActive() const { return bDoubletap || bTeleport || bRecharge || bSpeedhack; }

	bool MeleeDoubletapCheck(CBaseEntity* pLocal);	// checks if we WILL doubletap, used by melee aimbot from AimbotMelee.cpp
	void CLMove(float accumulated_extra_samples, bool bFinalTick);	// to be run from CL_Move.cpp
	void StartPrediction(CUserCmd* pCmd);
	void EndPrediction(CUserCmd* pCmd);
	void CreateMove(CUserCmd* pCmd, bool* pSendPacket);				// to be run from the CHLClient CreateMove hook
	void FinalizeAntiWarp(CUserCmd* pCmd);
	void Reset();
	void DrawDebug();
};

ADD_FEATURE(CTickshiftHandler, Ticks)
