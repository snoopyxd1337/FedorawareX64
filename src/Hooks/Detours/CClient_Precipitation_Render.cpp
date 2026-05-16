#include "../Hooks.h"

MAKE_HOOK(CClient_Precipitation_Render, nullptr, void, __fastcall,
	void* ecx, void* edx)
{
	return Hook.Original<FN>()(ecx, edx);
}

