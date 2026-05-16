#include "../Hooks.h"

MAKE_HOOK(CClient_Precipitation_SimulateRain, nullptr, bool, __fastcall,
	void* ecx, void* edx, void* pParticle, float dt)
{
	return Hook.Original<FN>()(ecx, edx, pParticle, dt);
}

