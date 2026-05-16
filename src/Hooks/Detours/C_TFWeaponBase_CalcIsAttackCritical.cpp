#include "../Hooks.h"

static int s_iCurrentSeed = -1;

MAKE_HOOK(C_TFWeaponBase_CalcIsAttackCritical, S::CTFWeaponBase_CalcIsAttackCritical(), void, __fastcall, void* ecx)
{
	const auto& pLocal = g_EntityCache.GetLocal();
	const auto pWeapon = reinterpret_cast<CBaseCombatWeapon*>(ecx);
	if (!pLocal || !pWeapon)
	{
		return Hook.Original<FN>()(ecx);
	}

	const auto nPreviousWeaponMode = pWeapon->m_iWeaponMode();
	pWeapon->m_iWeaponMode() = 0;

	if (I::Prediction->m_bFirstTimePredicted)
	{
		Hook.Original<FN>()(ecx);
		s_iCurrentSeed = pWeapon->m_iCurrentSeed();
	}
	else
	{
		const float flOldCritTokenBucket = pWeapon->m_flCritTokenBucket();
		const int nOldCritChecks = pWeapon->m_nCritChecks();
		const int nOldCritSeedRequests = pWeapon->m_nCritSeedRequests();
		const float flOldLastRapidFireCritCheckTime = pWeapon->m_flLastRapidFireCritCheckTime();
		const float flOldCritTime = pWeapon->m_flCritTime();

		Hook.Original<FN>()(ecx);

		pWeapon->m_flCritTokenBucket() = flOldCritTokenBucket;
		pWeapon->m_nCritChecks() = nOldCritChecks;
		pWeapon->m_nCritSeedRequests() = nOldCritSeedRequests;
		pWeapon->m_flLastRapidFireCritCheckTime() = flOldLastRapidFireCritCheckTime;
		pWeapon->m_flCritTime() = flOldCritTime;
		pWeapon->m_iCurrentSeed() = s_iCurrentSeed;
	}

	pWeapon->m_iWeaponMode() = nPreviousWeaponMode;
}
