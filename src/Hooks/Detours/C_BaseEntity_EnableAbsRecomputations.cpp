#include "../Hooks.h"

MAKE_HOOK(C_BaseAnimating_EnableAbsRecomputations, nullptr, void, __cdecl,
	bool bEnable)
{
	return Hook.Original<FN>()(bEnable);
}

