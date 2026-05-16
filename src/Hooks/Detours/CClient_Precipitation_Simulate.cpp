#include "../Hooks.h"

MAKE_HOOK(CClient_Precipitation_Simulate, nullptr, void, __fastcall,
	void* ecx, void* edx, float dt)
{
	return Hook.Original<FN>()(ecx, edx, dt);
}

