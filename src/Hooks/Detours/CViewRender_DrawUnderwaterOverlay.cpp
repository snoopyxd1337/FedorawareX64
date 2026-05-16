#include "../Hooks.h"

MAKE_HOOK(CViewRender_DrawUnderwaterOverlay, S::CViewRender_DrawUnderwaterOverlay(), void, __fastcall, void* eax)
{
	if (Vars::Visuals::RemoveScreenOverlays.Value && !(I::EngineClient->IsTakingScreenshot() && Vars::Visuals::CleanScreenshots.Value))
	{
		return;
	}
	return Hook.Original<FN>()(eax);
}
