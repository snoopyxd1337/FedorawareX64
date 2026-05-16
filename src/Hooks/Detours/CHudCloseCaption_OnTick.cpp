#include "../Hooks.h"

MAKE_HOOK(CHudCloseCaption_OnTick, nullptr, void, __fastcall,
	void* ecx, void* edx)
{
	return Hook.Original<FN>()(ecx, edx);
}

