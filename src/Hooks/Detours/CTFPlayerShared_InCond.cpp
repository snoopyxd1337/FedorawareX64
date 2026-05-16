#include "../Hooks.h"

namespace S
{
	MAKE_SIGNATURE(PlayerShouldDraw, CLIENT_DLL, "84 C0 74 ? 32 C0 48 8B 74 24", 0x0);
	MAKE_SIGNATURE(WearableShouldDraw, CLIENT_DLL, "84 C0 0F 85 ? ? ? ? 41 BF", 0x0);
	MAKE_SIGNATURE(HudScopeShouldDraw, CLIENT_DLL, "84 C0 74 ? 48 8B CB E8 ? ? ? ? 48 85 C0 74 ? 48 8B CB E8 ? ? ? ? 48 8B C8 48 8B 10 FF 92 ? ? ? ? 83 F8 ? 0F 94 C0", 0x0);
}

MAKE_HOOK(CTFPlayerShared_InCond, S::CTFPlayerShared_InCond(), bool, __fastcall, void* ecx, ETFCond nCond)
{
	static const auto dwPlayerShouldDraw = S::PlayerShouldDraw();
	static const auto dwWearableShouldDraw = S::WearableShouldDraw();
	static const auto dwHudScopeShouldDraw = S::HudScopeShouldDraw();

	const auto dwRetAddr = reinterpret_cast<uintptr_t>(_ReturnAddress());

	//static std::map<void*, bool> retaddrs;

	//if (retaddrs.find(retad) == retaddrs.end())
	//{
	//	retaddrs[retad] = true;
	//	I::CVars->ConsolePrintf("%p\n", retad);
	//}

	if (nCond == TF_COND_ZOOMED)
	{
		if (dwRetAddr == dwPlayerShouldDraw ||
			dwRetAddr == dwWearableShouldDraw ||
			(Vars::Visuals::RemoveScope.Value && dwRetAddr == dwHudScopeShouldDraw))
		{
			return false;
		}
	}

	auto GetOuter = [&ecx]() -> CBaseEntity*
	{
		static const auto dwShared = g_NetVars.get_offset("DT_TFPlayer", "m_Shared");
		static const auto dwBombHeadStage = g_NetVars.get_offset(
			"DT_TFPlayer", "m_Shared", "m_nHalloweenBombHeadStage");
		static const auto dwOff = (dwBombHeadStage - dwShared) + 0x4;
		return *reinterpret_cast<CBaseEntity**>(reinterpret_cast<uintptr_t>(ecx) + dwOff);
	};

	//Compare team's, removing team's taunt is useless
	if (nCond == TF_COND_TAUNTING && Vars::Visuals::RemoveTaunts.Value)
	{
		if (const auto& pLocal = g_EntityCache.GetLocal())
		{
			if (const auto& pEntity = GetOuter())
			{
				if (pEntity->GetTeamNum() != pLocal->GetTeamNum())
				{
					return false;
				}
			}
		}
	}

	//Just compare entity ptr's, filtering out local is enough. Also prevents T pose.
	if (nCond == TF_COND_DISGUISED && Vars::Visuals::RemoveDisguises.Value && g_EntityCache.GetLocal() != GetOuter())
	{
		return false;
	}

	//if (nCond == TF_COND_FEIGN_DEATH) { // stops the death chat message when you kill a dead ringer?
	//	return false;
	//}

	return Hook.Original<FN>()(ecx, nCond);
}
