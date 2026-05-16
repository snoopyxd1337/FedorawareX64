#include "../Hooks.h"

MAKE_HOOK(CInterpolatedVarArrayBase__Interpolate, nullptr, void, __fastcall,
	void* ecx, void* edx, void* out, float frac, void* start, void* end)
{
	return Hook.Original<FN>()(ecx, edx, out, frac, start, end);
}

