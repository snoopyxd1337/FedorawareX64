#include "../Hooks.h"
#include "../../Features/Vars.h"

MAKE_HOOK(C_BaseEntity_GetInterpolationAmount, nullptr, float, __fastcall,
	void* ecx, void* edx, int flags)
{
	return Hook.Original<FN>()(ecx, edx, flags);
}

