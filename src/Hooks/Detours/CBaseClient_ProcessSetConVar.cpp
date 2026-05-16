#include "../Hooks.h"

MAKE_HOOK(CBaseClient_ProcessSetConVar, g_Pattern.Find("engine.dll", "48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 8B E9 48 8B D1"), bool, __fastcall,
	void* ecx, NET_SetConVar* msg)
{
	return /*Vars::Visuals::RemoveForcedConvars.Value ? true : */Hook.Original<FN>()(ecx, msg);
}

