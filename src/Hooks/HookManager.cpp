#include "HookManager.h"

#include <ranges>

#include "Hooks.h"
#include "../SDK/SDK.h"
#include "MenuHook/MenuHook.h"
#include "../Utils/Minidump/Minidump.h"

#include <unordered_set>

namespace Hooks
{
	namespace CPrediction_RunSimulation
	{
		extern CHook Hook;
		void Initialize();
	}
}

inline uintptr_t GetVFuncPtr(void* pBaseClass, unsigned int nIndex)
{
	if (!pBaseClass || !*static_cast<uintptr_t**>(pBaseClass))
	{
		return 0;
	}

	return (*static_cast<uintptr_t**>(pBaseClass))[nIndex];
}

bool InitHookProtected(CHook* hook)
{
	__try
	{
		hook->Init();
		return true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return false;
	}
}

CHook::CHook(const std::string& name, void* pInitFunction)
{
	this->m_Name = name;
	this->m_InitFunction = pInitFunction;
	g_HookManager.GetMapHooks()[name] = this;
}

void CHookManager::Release()
{
	MH_Uninitialize();
	WndProc::Unload();
}

void CHookManager::Init()
{
	MH_Initialize();
	{
		auto& hooks = GetMapHooks();
		FW_TRACE(std::format("[HOOK] explicit check name=CPrediction_RunSimulation registered={} object={}",
			hooks.contains("CPrediction_RunSimulation"),
			reinterpret_cast<uintptr_t>(&Hooks::CPrediction_RunSimulation::Hook)).c_str());
		if (!hooks.contains("CPrediction_RunSimulation"))
		{
			FW_TRACE("[HOOK] explicit init name=CPrediction_RunSimulation reason=not-present-in-hook-map");
			Hooks::CPrediction_RunSimulation::Hook.Init();
		}

		if (I::DirectXDevice)
		{
			WndProc::Init();
		}
		else
		{
#if defined(FW_DEBUG_DIAGNOSTICS)
			OutputDebugStringA("DirectXDevice is null; skipping WndProc init\n");
#endif
			FW_TRACE("[WARN] DirectXDevice is null; skipping WndProc init");
		}

		static const std::unordered_set<std::string> disabledHooks = {
			"CBaseClient_Connect",
			"CBaseClient_Disconnect",
			"CBaseClient_SendSignonData",
			"CEconItemSchema_GetItemDefinition",
			"CEconNotification_HasNewItems",
			"CInterpolatedVarArrayBase_Interpolate",
			"CheckSimpleMaterial",
			"ClientModeTFNormal_UpdateSteamRichPresence",
			"CL_SendMove",
			"CHudCloseCaption_OnTick",
			"CInterpolatedVarArrayBase__Extrapolate",
			"CInterpolatedVarArrayBase__Interpolate",
			"CL_LatchInterpolationAmount",
			"CLagCompensationManager_StartLagCompensation",
			"CMaterial_DeleteIfUnreferenced",
			"CNetGraphPanel_DrawTextFields",
			"CNewParticleEffect_DrawModel",
			"COcclusionSystem_IsOccluded",
			"CServerGameClients_ProcessUsercmds",
			"CSequenceTransitioner_CheckForSequenceChange",
			"CTFMatchSummary_OnTick",
			"CTFPartyClient_BCanRequestToJoinPlayer",
			"CTFPartyClient_OnInQueueChanged",
			"CTFPlayerInventory_OnHasNewItems",
			"CVoiceBanMgr_GetPlayerBan",
			"C_BaseAnimating_EnableAbsRecomputations",
			"C_BaseAnimating_InvalidateBoneCache",
			"C_BaseAnimating_SetAbsQueriesValid",
			"C_BaseCombatWeapon_AddToCritBucket",
			"C_BaseCombatWeapon_IsAllowedToWithdrawFromCritBucket",
			"C_BaseEntity_AddVisibleEntities",
			"C_BaseEntity_AddVar",
			"C_BaseEntity_CalcAimEntPositions",
			"C_BaseEntity_FireBullets",
			"C_BaseEntity_GetInterpolationAmount",
			"C_BaseEntity_InterpolateServerEntities",
			"C_BaseEntity_MarkAimEntsDirty",
			"C_BaseEntity_MoveToLastReceivedPosition",
			"C_BaseEntity_ToolRecordEntities",
			"C_BasePlayer_CalcFreezeCamView",
			"C_TFPlayer_FireEvent",
			"CClient_Precipitation_Render",
			"CClient_Precipitation_Simulate",
			"CClient_Precipitation_SimulateRain",
			"IsLocalPlayerUsingVisionFilterFlags",
			"MIX_PaintChannels",
			"Panel_PaintTraverse",
			"Surface_OnScreenSizeChanged",
			"ViewRender_PerformScreenSpaceEffects"
		};

		for (const auto hook : GetMapHooks() | std::views::values)
		{
			if (disabledHooks.contains(hook->GetName()))
			{
				FW_TRACE(("[WARN] Skipping disabled hook: " + hook->GetName()).c_str());
				continue;
			}

			FW_TRACE(("Initializing hook: " + hook->GetName()).c_str());
			if (!InitHookProtected(hook))
			{
				FW_TRACE(("[ERROR] Hook initialization threw SEH exception: " + hook->GetName() + " reason=address-expression-or-init-crashed").c_str());
				continue;
			}

			if (!hook->HasOriginal() && !hook->TargetWasNull())
			{
				FW_TRACE(("[ERROR] Hook did not install: " + hook->GetName()).c_str());
			}
		}
	}

	if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK)
	{
#if defined(FW_DEBUG_DIAGNOSTICS)
		OutputDebugStringA("MH failed to enable all hooks\n");
#endif
		FW_TRACE("[ERROR] MH failed to enable all hooks");
	}
	else
	{
		FW_TRACE("[OK] MH enabled all hooks");
	}
}
