#include "../Hooks.h"

MAKE_HOOK(CBaseClientState_ProcessSetConVar, g_Pattern.Find("engine.dll", "40 53 48 83 EC ? 48 8B D9 48 8B 49 ? 48 8B 01 FF 50 ? 84 C0 0F 85"), bool, __fastcall,
	void* ecx, NET_SetConVar* msg)
{
	return /*Vars::Visuals::RemoveForcedConvars.Value ? true : */Hook.Original<FN>()(ecx, msg);	//	weird crash
}

