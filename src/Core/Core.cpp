#include "Core.h"

#include "../Hooks/HookManager.h"
#include "../Hooks/PatchManager/PatchManager.h"

#include "../Features/NetVarHooks/NetVarHooks.h"
#include "../Features/Visuals/Visuals.h"
#include "../Features/Vars.h"
#include "../Features/TickHandler/TickHandler.h"

#include "../Features/Menu/Menu.h"
#include "../Features/Menu/ConfigManager/ConfigManager.h"
#include "../Features/Items/AttributeChanger/AttributeChanger.h"
#include "../Features/Commands/Commands.h"
#include "../Features/Discord/Discord.h"

#include "../Utils/Events/Events.h"
#include "../Utils/Minidump/Minidump.h"

void LoadDefaultConfig()
{
	// Load default visuals
	g_CFG.LoadVisual(g_CFG.GetCurrentVisuals());
	g_CFG.LoadConfig(g_CFG.GetCurrentConfig());

	g_Draw.RemakeFonts();

	F::Menu.ConfigLoaded = true;
}

void CCore::OnLoaded()
{
	FW_TRACE("CCore::OnLoaded begin");
	LoadDefaultConfig();
	FW_TRACE("CCore::OnLoaded default config loaded");

	I::Cvar->ConsoleColorPrintf(Vars::Menu::Colors::MenuAccent.Value, "%s Loaded!\n", Vars::Menu::CheatName.Value.c_str());
	I::EngineClient->ClientCmd_Unrestricted("play vo/items/wheatley_sapper/wheatley_sapper_attached14.mp3");
	FW_TRACE("CCore::OnLoaded console/audio done");

	// Check the DirectX version
	const int dxLevel = g_ConVars.FindVar("mat_dxlevel")->GetInt();
	if (dxLevel < 90)
	{
		FW_TRACE("[WARN] mat_dxlevel is below 90");
		MessageBoxA(nullptr, "Your DirectX version is too low!\nPlease use dxlevel 90 or higher", "dxlevel too low", MB_OK | MB_ICONWARNING);
	}
}

void CCore::Load()
{
	FW_TRACE("CCore::Load g_SteamInterfaces.Init begin");
	g_SteamInterfaces.Init();
	FW_TRACE("CCore::Load g_SteamInterfaces.Init done");

	FW_TRACE("CCore::Load g_Interfaces.Init begin");
	g_Interfaces.Init();
	FW_TRACE("CCore::Load g_Interfaces.Init done");

	FW_TRACE("CCore::Load g_NetVars.Init begin");
	g_NetVars.Init();
	FW_TRACE("CCore::Load g_NetVars.Init done");

	// Initialize hooks & memory stuff
	{
		FW_TRACE("CCore::Load g_HookManager.Init begin");
		g_HookManager.Init();
		FW_TRACE("CCore::Load g_HookManager.Init done");

		FW_TRACE("CCore::Load g_PatchManager.Init begin");
		g_PatchManager.Init();
		FW_TRACE("CCore::Load g_PatchManager.Init done");

		FW_TRACE("CCore::Load F::NetHooks.Init begin");
		F::NetHooks.Init();
		FW_TRACE("CCore::Load F::NetHooks.Init done");
	}

	FW_TRACE("CCore::Load g_ConVars.Init begin");
	g_ConVars.Init();
	FW_TRACE("CCore::Load g_ConVars.Init done");

	F::Ticks.Reset();

	FW_TRACE("CCore::Load F::Commands.Init begin");
	F::Commands.Init();
	FW_TRACE("CCore::Load F::Commands.Init done");

	FW_TRACE("CCore::Load F::DiscordRPC.Init begin");
	F::DiscordRPC.Init();
	FW_TRACE("CCore::Load F::DiscordRPC.Init done");

	FW_TRACE("CCore::Load g_Events.Setup begin");
	g_Events.Setup({
		"vote_cast", "player_changeclass", "player_connect", "player_hurt", "achievement_earned", "player_death", "vote_started", "teamplay_round_start", "player_spawn", "item_pickup", "scorestats_accumulated_update", "mvm_reset_stats"
	}); // all events @ https://github.com/tf2cheater2013/gameevents.txt
	FW_TRACE("CCore::Load g_Events.Setup done");

	OnLoaded();
	FW_TRACE("CCore::Load OnLoaded done");
	FW_TRACE("[OK] Fedoraware load completed");
}

void CCore::Unload()
{
	I::EngineClient->ClientCmd_Unrestricted("play vo/items/wheatley_sapper/wheatley_sapper_hacked02.mp3");
	G::UnloadWndProcHook = true;
	Vars::Visuals::SkyboxChanger.Value = false;
	Vars::Visuals::ThirdPerson.Value = false;

	Sleep(100);

	g_Events.Destroy();
	g_HookManager.Release();
	g_PatchManager.Restore();

	F::DiscordRPC.Shutdown();

	Sleep(100);

	F::Visuals.RestoreWorldModulation(); //needs to do this after hooks are released cuz UpdateWorldMod in FSN will override it
	I::Cvar->ConsoleColorPrintf(Vars::Menu::Colors::MenuAccent.Value, "%s Unloaded!\n", Vars::Menu::CheatName.Value.c_str());
}

bool CCore::ShouldUnload()
{
	const bool unloadKey = GetAsyncKeyState(VK_F11) & 0x8000;
	return unloadKey && !F::Menu.IsOpen;
}
