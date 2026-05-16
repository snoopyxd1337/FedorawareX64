#include "../Hooks.h"

//	xref C_BaseEntity_BaseInterpolatePart1
MAKE_HOOK(C_BaseEntity_Interpolate, S::CBaseEntity_Interpolate(), bool, __fastcall, void* ecx, float currentTime)
{
	CBaseEntity* pLocal = g_EntityCache.GetLocal();
	if (ecx == pLocal)
	{
		return G::Recharging ? true : Hook.Original<FN>()(ecx, currentTime);
	}

	return Vars::Misc::DisableInterpolation.Value ? true : Hook.Original<FN>()(ecx, currentTime);
}
