#include "../Hooks.h"

#include "../../Features/Resolver/Resolver.h"
#include "../../Features/Visuals/Visuals.h"
#include "../../Features/Items/AttributeChanger/AttributeChanger.h"
#include "../../Features/Menu/Playerlist/Playerlist.h"
#include "../../Features/Backtrack/Backtrack.h"
#include "../../Features/Aimbot/MovementSimulation/MovementSimulation.h"
#include "../../Features/Backtrack/Backtrack.h"
#include "../../Utils/FunctionalDebug/FunctionalDebug.h"

MAKE_HOOK(BaseClientDLL_FrameStageNotify, Utils::GetVFuncPtr(I::BaseClientDLL, 35), void, __fastcall,
		  void* ecx, EClientFrameStage curStage)
{
	FDBG_FLUSH("FrameStageNotify");
	FDBG_SCOPE_SLOW("Hook.BaseClientDLL_FrameStageNotify", FunctionalDebug::HOOK_SLOW_US);

	switch (curStage)
	{
		case EClientFrameStage::FRAME_RENDER_START:
		{
			G::PunchAngles = Vec3();

			if (const auto& pLocal = g_EntityCache.GetLocal())
			{
				// Remove punch effect
				{
					G::PunchAngles = pLocal->m_vecPunchAngle();	//	use in aimbot 
					if (Vars::Visuals::RemovePunch.Value) { pLocal->ClearPunchAngle(); }	//	visual no-recoil
				}
			}

			// Resolver
			FDBG_CALL("Feature.Resolver.FrameStageNotify", F::Resolver.FrameStageNotify());

			FDBG_CALL("Feature.Visuals.SkyboxChanger", F::Visuals.SkyboxChanger());

			break;
		}
	}

	FDBG_CALL("FrameStageNotify.Original", Hook.Original<FN>()(ecx, curStage));

	switch (curStage)
	{
		case EClientFrameStage::FRAME_NET_UPDATE_START:
		{
			FDBG_CALL("EntityCache.Clear", g_EntityCache.Clear());

			break;
		}


		case EClientFrameStage::FRAME_NET_UPDATE_POSTDATAUPDATE_START:
		{
			FDBG_CALL("Feature.AttributeChanger.Run", F::AttributeChanger.Run());

			break;
		}


		case EClientFrameStage::FRAME_NET_UPDATE_END:
		{
			bool entityCacheFilled = false;
			FDBG_CALL("EntityCache.Fill", entityCacheFilled = g_EntityCache.Fill());
			if (!entityCacheFilled)
			{
				FDBG_SKIP("FrameStageNotify.NET_UPDATE_END.Features", "EntityCache.Fill returned false");
				break;
			}

			FDBG_CALL("Feature.Backtrack.FrameStageNotify", F::Backtrack.FrameStageNotify());
			FDBG_CALL("Feature.MoveSim.FillVelocities", F::MoveSim.FillVelocities());
			FDBG_CALL("Feature.Visuals.FillSightlines", F::Visuals.FillSightlines());
			G::LocalSpectated = false;
			FDBG_CALL("Feature.Visuals.PruneBulletTracers", F::Visuals.PruneBulletTracers());
			if (const auto& pLocal = g_EntityCache.GetLocal())
			{
				for (const auto& teammate : g_EntityCache.GetGroup(EGroupType::PLAYERS_TEAMMATES))
				{
					if (teammate->IsAlive() || g_EntityCache.IsFriend(teammate->GetIndex()))
					{
						continue;
					}

					const CBaseEntity* pObservedPlayer = I::ClientEntityList->GetClientEntityFromHandleSafe(teammate->GetObserverTarget());

					if (pObservedPlayer == pLocal)
					{
						G::LocalSpectated = true;
						break;
					}
				}
			}

			for (int i = 0; i < I::EngineClient->GetMaxClients(); i++)
			{
				if (const auto& player = I::ClientEntityList->GetClientEntity(i))
				{
					const VelFixRecord record = { player->m_vecOrigin(), player->m_fFlags(), player->GetSimulationTime() };
					G::VelFixRecords[player] = record;
				}
			}

			FDBG_CALL("Feature.PlayerList.UpdatePlayers", F::PlayerList.UpdatePlayers());
			break;
		}

		case EClientFrameStage::FRAME_RENDER_START:
		{
			if (!G::UnloadWndProcHook)
			{
				if (G::ShouldUpdateMaterialCache)
				{
					FDBG_CALL("Feature.Visuals.ClearMaterialHandles", F::Visuals.ClearMaterialHandles());
					FDBG_CALL("Feature.Visuals.StoreMaterialHandles", F::Visuals.StoreMaterialHandles());
					G::ShouldUpdateMaterialCache = false;
				}
				if (Vars::Visuals::Rain.Value > 0)
				{
					FDBG_CALL("Feature.Visuals.Rain.Run", F::Visuals.rain.Run());
				}
				else
				{
					FDBG_SKIP("Feature.Visuals.Rain.Run", "Vars::Visuals::Rain <= 0");
				}

				FDBG_CALL("Feature.Visuals.ModulateWorld", F::Visuals.ModulateWorld());
			}
			else
			{
				FDBG_SKIP("FrameStageNotify.RENDER_START.Features", "UnloadWndProcHook true");
			}
			break;
		}
	}
}
