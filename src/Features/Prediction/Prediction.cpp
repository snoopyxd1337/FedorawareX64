#include "Prediction.h"

namespace
{
	bool RunPredictionSimulation(CBaseEntity* pLocal, CUserCmd* pCmd, CMoveData* pMoveData)
	{
		__try
		{
			I::Prediction->SetupMove(pLocal, pCmd, I::MoveHelper, pMoveData);
			I::GameMovement->ProcessMovement(pLocal, pMoveData);
			I::Prediction->FinishMove(pLocal, pCmd, pMoveData);
			return true;
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			return false;
		}
	}
}

int CEnginePrediction::GetTickbase(CUserCmd* pCmd, CBaseEntity* pLocal)
{
	static int nTick = 0;
	static CUserCmd* pLastCmd = nullptr;

	if (pCmd)
	{
		if (!pLastCmd || pLastCmd->hasbeenpredicted)
			nTick = pLocal->GetTickBase();

		else nTick++;

		pLastCmd = pCmd;
	}

	return nTick;
}

void CEnginePrediction::Start(CUserCmd* pCmd)
{
	// credits for some sigs https://www.unknowncheats.me/forum/3333826-post1.html

	m_bStarted = false;
	CBaseEntity* pLocal = g_EntityCache.GetLocal();

	CBaseCombatWeapon* pWeapon = pLocal ? pLocal->GetActiveWeapon() : nullptr;
	CBaseCombatWeapon* pCachedWeapon = g_EntityCache.GetWeapon();

	if (pCmd && pLocal && pLocal->IsAlive() && pWeapon && pCachedWeapon && pWeapon == pCachedWeapon
		&& !G::ShouldShift && !G::Teleporting
		&& I::GlobalVars && I::Prediction && I::MoveHelper && I::GameMovement && I::RandomSeed)
	{
		pLocal->SetCurrentCmd(pCmd);

		oldrandomseed = *I::RandomSeed;
		*I::RandomSeed = MD5_PseudoRandom(pCmd->command_number) & std::numeric_limits<int>::max();


		m_fOldCurrentTime = I::GlobalVars->curtime;
		m_fOldFrameTime = I::GlobalVars->frametime;
		m_nOldTickCount = I::GlobalVars->tickcount;
		m_bStarted = true;

		const int nOldTickBase = pLocal->GetTickBase();
		const bool bOldIsFirstPrediction = I::Prediction->m_bFirstTimePredicted;
		const bool bOldInPrediction = I::Prediction->m_bInPrediction;

		I::GlobalVars->curtime = TICKS_TO_TIME(GetTickbase(pCmd, pLocal));
		I::GlobalVars->frametime = (I::Prediction->m_bEnginePaused ? 0.0f : TICK_INTERVAL);
		I::GlobalVars->tickcount = GetTickbase(pCmd, pLocal);

		I::Prediction->m_bFirstTimePredicted = false;
		I::Prediction->m_bInPrediction = true;

		I::GameMovement->StartTrackPredictionErrors(pLocal);

		//if (pCmd->weaponselect) {
		//	if (CBaseCombatWeapon* pWeapon = pLocal->GetActiveWeapon()) { 
		//		pLocal->SelectItem(pWeapon->GetName(), pCmd->weaponsubtype); 
		//	}
		//}

		pLocal->UpdateButtonState(pCmd->buttons);

		I::Prediction->SetLocalViewAngles(pCmd->viewangles);

		I::MoveHelper->SetHost(pLocal);

		m_MoveData = {};
		const bool bSimulationOk = RunPredictionSimulation(pLocal, pCmd, &m_MoveData);

		I::MoveHelper->SetHost(nullptr);
		if (bSimulationOk)
		{
			I::GameMovement->FinishTrackPredictionErrors(pLocal);
		}
		pLocal->SetTickBase(nOldTickBase);

		I::Prediction->m_bInPrediction = bOldInPrediction;
		I::Prediction->m_bFirstTimePredicted = bOldIsFirstPrediction;

		if (!bSimulationOk)
		{
			I::GlobalVars->curtime = m_fOldCurrentTime;
			I::GlobalVars->frametime = m_fOldFrameTime;
			I::GlobalVars->tickcount = m_nOldTickCount;
			pLocal->SetCurrentCmd(nullptr);
			*I::RandomSeed = -1;
			m_bStarted = false;
		}
	}
}

void CEnginePrediction::End(CUserCmd* pCmd)
{
	if (!m_bStarted)
	{
		return;
	}

	CBaseEntity* pLocal = g_EntityCache.GetLocal();

	if (pCmd && pLocal && pLocal->IsAlive() && !G::ShouldShift && !G::Teleporting
		&& I::GlobalVars && I::MoveHelper && I::RandomSeed)
	{
		I::MoveHelper->SetHost(nullptr);

		I::GlobalVars->curtime = m_fOldCurrentTime;
		I::GlobalVars->frametime = m_fOldFrameTime;
		I::GlobalVars->tickcount = m_nOldTickCount;
		pLocal->SetCurrentCmd(nullptr);

		*I::RandomSeed = -1;
	}
	m_bStarted = false;
}
