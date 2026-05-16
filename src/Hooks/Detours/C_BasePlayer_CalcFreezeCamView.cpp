#include "../Hooks.h"

MAKE_HOOK(C_BasePlayer_CalcFreezeCamView, nullptr, void, __fastcall,
		  void* ecx, void* edx, Vector& eyeOrigin, QAngle& eyeAngles, float& fov)
{
	Hook.Original<FN>()(ecx, edx, eyeOrigin, eyeAngles, fov);
}
