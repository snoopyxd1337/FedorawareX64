#include "../Hooks.h"

namespace
{
	int GetSafePredictionChokedCommands()
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
}

/*
	https://github.com/perilouswithadollarsign/cstrike15_src/blob/f82112a2388b841d72cb62ca48ab1846dfcc11c8/game/server/player.cpp#L3380
	- bradley wuzz here fr fr on goh
*/
inline void TickCorrection(const int iSimTicks, CBaseEntity* pLocal) {
	const ConVar* svClockCorrection = I::Cvar ? I::Cvar->FindVar("sv_clockcorrection_msecs") : nullptr;
	const auto netChannel = I::EngineClient ? I::EngineClient->GetNetChannelInfo() : nullptr;
	if (!svClockCorrection || !netChannel)
	{
		return;
	}

	const int iClockCorrect = TIME_TO_TICKS(svClockCorrection->GetFloat());

	const int iIdealEnd = I::GlobalVars->tickcount + TIME_TO_TICKS(netChannel->GetLatency(FLOW_OUTGOING)) + iClockCorrect;
	const int iActualEnd = pLocal->GetTickBase() + iSimTicks;

	if (iActualEnd > iIdealEnd + iClockCorrect ||
		iActualEnd < iIdealEnd - iClockCorrect) {
		pLocal->SetTickBase(iIdealEnd - iSimTicks + 1);
		I::GlobalVars->curtime = TICKS_TO_TIME(pLocal->GetTickBase());
	}
}

MAKE_HOOK(Prediction_RunCommand, Utils::GetVFuncPtr(I::Prediction, 17), void, __fastcall,
		  void* ecx, CBaseEntity* pEntity, CUserCmd* pCmd, CMoveHelper* pMoveHelper)
{
	CBaseEntity* pLocal = g_EntityCache.GetLocal();

	if (pLocal && pLocal == pEntity && pCmd) {
		if (G::Recharging) {
			return;
		}

		if (G::ShouldShift || G::Teleporting) {
			const int iTickbaseBackup = pLocal->GetTickBase();
			const float flCurtimeBackup = I::GlobalVars->curtime;

			TickCorrection(GetSafePredictionChokedCommands() + G::ShiftedTicks, pLocal);
			Hook.Original<FN>()(ecx, pEntity, pCmd, pMoveHelper);

			pLocal->SetTickBase(iTickbaseBackup);
			I::GlobalVars->curtime = flCurtimeBackup;
		}
		else {
			Hook.Original<FN>()(ecx, pEntity, pCmd, pMoveHelper);
		}
	}
	else {
		Hook.Original<FN>()(ecx, pEntity, pCmd, pMoveHelper);
	}

	//credits: KGB
	if (pLocal && !pLocal->InCond(TF_COND_HALLOWEEN_KART)) {
		if (!pCmd->hasbeenpredicted && pEntity == pLocal) {
			if (CTFPlayerAnimState* pAnimState = pLocal->GetAnimState()) {
				const float flOldFrameTime = I::GlobalVars->frametime;
				I::GlobalVars->frametime = I::Prediction->m_bEnginePaused ? 0.0f : TICK_INTERVAL;
				pAnimState->Update(pCmd->viewangles.y, pCmd->viewangles.x);
				pLocal->FrameAdvance(I::GlobalVars->frametime);
				I::GlobalVars->frametime = flOldFrameTime;
			}
		}
	}
}
