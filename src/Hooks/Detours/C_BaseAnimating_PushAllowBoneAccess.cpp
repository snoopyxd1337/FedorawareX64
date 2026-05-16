#include "../Hooks.h"

MAKE_HOOK(C_BaseAnimating_PushAllowBoneAccess, g_Pattern.Find("client.dll", "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 0F B6 F1 0F B6 FA 48 8B D9 E8 ? ? ? ? 48 8B C8 C7 44 24 ? ? ? ? ? 4C 8D 0D ? ? ? ? 33 D2 4C 8D 05 ? ? ? ? E8 ? ? ? ? 84 C0"), void, __cdecl,
	bool bAllowForNormalModels, bool bAllowForViewModels, char const *tagPush)
{
	return Hook.Original<FN>()(bAllowForNormalModels, bAllowForViewModels, tagPush);
}

