#include "../Hooks.h"

MAKE_HOOK(CL_LatchInterpolationAmount, nullptr, void, __fastcall,
	void* ecx, void* edx)
{
	return Hook.Original<FN>()(ecx, edx);
}

