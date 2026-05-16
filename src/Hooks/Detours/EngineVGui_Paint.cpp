#include "../Hooks.h"

#include "../../SDK/Includes/icons.h"
#include "../../Features/SpyWarning/SpyWarning.h"
#include "../../Features/PlayerArrows/PlayerArrows.h"
#include "../../Features/ESP/ESP.h"
#include "../../Features/Misc/Notifications/Notifications.h"
#include "../../Features/Visuals/Visuals.h"
#include "../../Features/CritHack/CritHack.h"
#include "../../Features/Menu/Menu.h"
#include "../../Features/Menu/SpectatorList/SpectatorList.h"
#include "../../Features/Radar/Radar.h"
#include "../../Features/Followbot/Followbot.h"
#include "../../Features/AutoQueue/AutoQueue.h"
#include "../../Features/Menu/MaterialEditor/MaterialEditor.h"
#include "../../Features/Menu/Playerlist/Playerlist.h"
#include "../../Utils/FunctionalDebug/FunctionalDebug.h"

namespace S
{
	MAKE_SIGNATURE(StartDrawing, MATSURFACE_DLL, "40 53 56 57 48 83 EC ? 48 8B F9 80 3D", 0x0);
	MAKE_SIGNATURE(FinishDrawing, MATSURFACE_DLL, "40 53 48 83 EC ? 33 C9", 0x0);
}

MAKE_HOOK(EngineVGui_Paint, Utils::GetVFuncPtr(I::EngineVGui, 14), void, __fastcall,
		  void* ecx, int iMode)
{
	FDBG_FLUSH("Paint");
	FDBG_SCOPE_SLOW("Hook.EngineVGui_Paint", FunctionalDebug::HOOK_SLOW_US);

	static auto StartDrawing = reinterpret_cast<void(*)(void*)>(S::StartDrawing());
	static auto FinishDrawing = reinterpret_cast<void(*)(void*)>(S::FinishDrawing());

	const auto bCleanScreenshot = Vars::Visuals::CleanScreenshots.Value && I::EngineClient->IsTakingScreenshot();

	const auto UpdateScreenSize = []()
	{
		FDBG_CALL("Paint.ScreenSize.Update", g_ScreenSize.Update());
	};

	const auto UpdateWorldToScreen = []()
	{
		CViewSetup viewSetup = {};

		bool gotPlayerView = false;
		FDBG_CALL("Paint.BaseClientDLL.GetPlayerView", gotPlayerView = I::BaseClientDLL->GetPlayerView(viewSetup));
		if (gotPlayerView)
		{
			VMatrix worldToView = {}, viewToProjection = {}, worldToPixels = {};
			FDBG_CALL("Paint.RenderView.GetMatricesForView", I::RenderView->GetMatricesForView(viewSetup, &worldToView, &viewToProjection,
																								&G::WorldToProjection, &worldToPixels));
		}
		else
		{
			FDBG_SKIP("Paint.RenderView.GetMatricesForView", "GetPlayerView returned false");
		}
	};

	const auto BeginDrawing = [&]() -> bool
	{
		if (!StartDrawing || !FinishDrawing)
		{
			FDBG_ERROR("Paint.Drawing", "StartDrawing or FinishDrawing signature missing");
			return false;
		}

		FDBG_CALL("Paint.StartDrawing", StartDrawing(I::VGuiSurface));
		return true;
	};

	const auto EndDrawing = [&]()
	{
		I::VGuiSurface->DrawSetAlphaMultiplier(1.0f);
		FDBG_CALL("Paint.FinishDrawing", FinishDrawing(I::VGuiSurface));
	};

	const auto InitIcons = []()
	{
		static bool bInitIcons = false;
		if (bInitIcons)
			return;

		for (int nIndex = 0; nIndex < ICONS::TEXTURE_AMOUNT; nIndex++)
		{
			ICONS::ID[nIndex] = -1;
			g_Draw.Texture(-200, 0, 18, 18, Colors::White, nIndex);
		}

		bInitIcons = true;
	};

	if (iMode & PAINT_INGAMEPANELS)
	{
		if (!bCleanScreenshot)
		{
			UpdateScreenSize();

			if (I::EngineClient->IsInGame())
				UpdateWorldToScreen();

			if (BeginDrawing())
			{
				InitIcons();

				if (I::EngineClient->IsInGame())
				{
					FDBG_CALL("Feature.RSChat.Draw", F::RSChat.Draw());
					FDBG_CALL("Feature.ESP.Draw", F::ESP.Draw());
					FDBG_CALL("Feature.Visuals.Draw", F::Visuals.Draw());
					FDBG_CALL("Feature.PlayerArrows.Draw", F::PlayerArrows.Draw());
					FDBG_CALL("Feature.Followbot.Draw", F::Followbot.Draw());
					FDBG_CALL("Feature.SpectatorList.Draw", F::SpectatorList.Draw());
					FDBG_CALL("Feature.CritHack.Draw", F::CritHack.Draw());
					FDBG_CALL("Feature.Radar.Draw", F::Radar.Draw());
					FDBG_CALL("Feature.AutoQueue.Run", F::AutoQueue.Run());
					FDBG_CALL("Feature.SpyWarning.Run", F::SpyWarning.Run());
					FDBG_CALL("Feature.PlayerList.Run", F::PlayerList.Run());
				}
				else
				{
					FDBG_SKIP("Paint.InGameFeatures", "EngineClient not in game");
				}

				EndDrawing();
			}
		}
		else
		{
			FDBG_SKIP("Paint.InGamePanels", "clean screenshot active");
		}
	}
	else
	{
		FDBG_SKIP("Paint.InGamePanels", "paint mode does not include PAINT_INGAMEPANELS");
	}

	FDBG_CALL("Paint.Original", Hook.Original<FN>()(ecx, iMode));

	if (iMode & PAINT_UIPANELS)
	{
		if (bCleanScreenshot)
		{
			FDBG_SKIP("Paint.UIPanels", "clean screenshot active");
			return;
		}

		UpdateScreenSize();

		if (!BeginDrawing())
			return;

		{
			InitIcons();

			// Main Menu stuff
			if (I::EngineVGui->IsGameUIVisible())
			{
				if (!I::EngineClient->IsInGame())
				{
					static time_t curTime = time(nullptr);
					static tm* curCalTime = localtime(&curTime);

					if (F::Menu.IsOpen)
					{
						const auto& menuFont = g_Draw.GetFont(FONT_MENU);
						g_Draw.String(menuFont, 5, g_ScreenSize.h - 2  - Vars::Fonts::FONT_MENU::nTall.Value, { 200, 200, 200, 255 }, ALIGN_DEFAULT, L"Build of %hs", __DATE__ " " __TIME__);
						if (curCalTime->tm_mon == 11 && curCalTime->tm_mday == 25) //this *probably* works
						{
							g_Draw.String(menuFont, g_ScreenSize.c, 150, Utils::Rainbow(), ALIGN_CENTERHORIZONTAL, "MERRY CHRISTMAS!!!!!!!");
						}
					}
					if(Vars::Menu::DrawWeather.Value)
					{ 
						if (curCalTime->tm_mon == 11 || curCalTime->tm_mon == 0 || curCalTime->tm_mon == 1) //december, january, february (winter months)
						{ //this method also sucks for getting the 3 months and can probably be shortened
							FDBG_CALL("Feature.Visuals.DrawMenuSnow", F::Visuals.DrawMenuSnow());
						}

						if (curCalTime->tm_mon == 8 || curCalTime->tm_mon == 9 || curCalTime->tm_mon == 10)
						{
							FDBG_CALL("Feature.Visuals.DrawMenuRain", F::Visuals.DrawMenuRain());
						}
					}
				}
			}

			FDBG_CALL("Feature.Notifications.Draw", F::Notifications.Draw());
		}

		EndDrawing();
	}
	else
	{
		FDBG_SKIP("Paint.UIPanels", "paint mode does not include PAINT_UIPANELS");
	}
}
