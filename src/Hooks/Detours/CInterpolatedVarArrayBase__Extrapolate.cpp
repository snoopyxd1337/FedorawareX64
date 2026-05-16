#include "../Hooks.h"

MAKE_HOOK(CInterpolatedVarArrayBase__Extrapolate, nullptr, void, __fastcall,
	void* ecx, void* edx, void* pOut, void* pOld, void* pNew, float flDestinationTime, float flMaxExtrapolationAmount)
{
	return Hook.Original<FN>()(ecx, edx, pOut, pOld, pNew, flDestinationTime, flMaxExtrapolationAmount);
}

