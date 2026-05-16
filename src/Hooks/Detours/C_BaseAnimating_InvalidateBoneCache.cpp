#include "../Hooks.h"

MAKE_HOOK(C_BaseAnimating_InvalidateBoneCache, nullptr, void, __fastcall,
	void* ecx)
{
	return Hook.Original<FN>()(ecx);
}

