#include "../Hooks.h"
#include "../HookManager.h"



MAKE_HOOK(C_BaseAnimating_SetupBones, g_Pattern.Find("client.dll", "48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 54 41 55 41 56 41 57 48 83 EC ? 48 8B 05 ? ? ? ? 48 8B DA"), bool, __fastcall,
	void* ecx, matrix3x4* pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime)
{
	return Hook.Original<FN>()(ecx, pBoneToWorldOut, nMaxBones, boneMask, currentTime);
}

