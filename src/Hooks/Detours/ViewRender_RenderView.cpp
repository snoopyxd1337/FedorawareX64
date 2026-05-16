#include "../Hooks.h"

#include "../../Features/Camera/CameraWindow.h"
#include "../../Features/Glow/Glow.h"
#include "../../Features/Chams/Chams.h"
#include "../../Features/Chams/DMEChams.h"
#include "../../Features/Items/AttributeChanger/AttributeChanger.h"
#include "../../Utils/FunctionalDebug/FunctionalDebug.h"

#include <mutex>

MAKE_HOOK(ViewRender_RenderView, Utils::GetVFuncPtr(I::ViewRender, 6), void, __fastcall,
		  void* ecx, const CViewSetup& view, ClearFlags_t nClearFlags, RenderViewInfo_t whatToDraw)
{
	FDBG_FLUSH("RenderView");
	FDBG_SCOPE_SLOW("Hook.ViewRender_RenderView", FunctionalDebug::HOOK_SLOW_US);

	static std::once_flag onceFlag;
	std::call_once(onceFlag, []
				   {
					   FDBG_CALL("Feature.Glow.Init", F::Glow.Init());
					   FDBG_CALL("Feature.Chams.Init", F::Chams.Init());
					   FDBG_CALL("Feature.DMEChams.Init", F::DMEChams.Init());
					   FDBG_CALL("Feature.CameraWindow.Init", F::CameraWindow.Init());
					   FDBG_CALL("Feature.AttributeChanger.Init", F::AttributeChanger.Init());
				   });

	const bool bProjectileCam = Vars::Visuals::ProjectileCameraKey.Value && GetAsyncKeyState(Vars::Visuals::ProjectileCameraKey.Value) & 0x8000 && !g_EntityCache.GetGroup(EGroupType::LOCAL_PROJECTILES).empty();
	const bool bFreeCam = G::FreecamActive && Vars::Visuals::FreecamKey.Value && GetAsyncKeyState(Vars::Visuals::FreecamKey.Value) & 0x8000;

	if (bFreeCam || bProjectileCam) {
		CViewSetup tCustomView{};
		memcpy(&tCustomView, &view, sizeof(CViewSetup));

		//	Projectile Camera (:vomit:)
		if (bProjectileCam) {
			CBaseEntity* pLocal = g_EntityCache.GetLocal();
			CBaseEntity* pFurthest = nullptr;
			for (CBaseEntity* pEntity : g_EntityCache.GetGroup(EGroupType::LOCAL_PROJECTILES)) {
				if (!pFurthest || pEntity->GetAbsOrigin().DistTo(pLocal->GetAbsOrigin()) > pFurthest->GetAbsOrigin().DistTo(pLocal->GetAbsOrigin())) {
					pFurthest = pEntity;
					continue;
				}
			}
			tCustomView.origin = pFurthest->GetAbsOrigin();
		}
		// Handle freecam position
		else if (bFreeCam) {
			tCustomView.origin = G::FreecamPos;
		}

		FDBG_CALL("RenderView.Original.CustomView", Hook.Original<void(*)(void*, const CViewSetup&, int, int)>()(ecx, tCustomView, nClearFlags, whatToDraw));
		if (!(I::EngineClient->IsTakingScreenshot() && Vars::Visuals::CleanScreenshots.Value)) { FDBG_CALL("Feature.CameraWindow.RenderView", F::CameraWindow.RenderView(ecx, view)); }
		else { FDBG_SKIP("Feature.CameraWindow.RenderView", "clean screenshot active"); }
		return;
	}
	FDBG_CALL("RenderView.Original", Hook.Original<void(*)(void*, const CViewSetup&, int, int)>()(ecx, view, nClearFlags, whatToDraw));
	if (!(I::EngineClient->IsTakingScreenshot() && Vars::Visuals::CleanScreenshots.Value)) { FDBG_CALL("Feature.CameraWindow.RenderView", F::CameraWindow.RenderView(ecx, view)); }
	else { FDBG_SKIP("Feature.CameraWindow.RenderView", "clean screenshot active"); }
}
