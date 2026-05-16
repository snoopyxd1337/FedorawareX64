#include "../Hooks.h"

MAKE_HOOK(CLagCompensationManager_StartLagCompensation, nullptr, void, __fastcall,
		  void* ecx, void* edx, int player, CUserCmd* pCmd)
{
	//const float flLerpTime = (*(float*)((DWORD)player + 2756));
	//Utils::ConLog("CLagCompensationManager_StartLagCompensation", tfm::format("flLerpTime = %.3f", flLerpTime).c_str(), { 0, 222, 255, 255 });
	return Hook.Original<FN>()(ecx, edx, player, pCmd);
}

