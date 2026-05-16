#include "../Hooks.h"
#include "../../Features/CritHack/CritHack.h"

MAKE_HOOK(C_BaseCombatWeapon_AddToCritBucket, S::CBaseCombatWeapon_AddToCritBucket(), void, __fastcall, void* ecx, float flAmount)
{
	if (F::CritHack.ProtectData)
	{
		return;
	}

	return Hook.Original<FN>()(ecx, flAmount);
}
