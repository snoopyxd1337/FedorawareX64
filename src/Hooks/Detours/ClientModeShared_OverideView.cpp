#include "../Hooks.h"

#include "../../Features/Visuals/Visuals.h"

MAKE_HOOK(ClientModeShared_OverrrideView, Utils::GetVFuncPtr(I::ClientModeShared, 16), void, __fastcall,
		  void* ecx, CViewSetup* pView)
{
	Hook.Original<FN>()(ecx, pView);

	if (I::EngineClient->IsTakingScreenshot() && Vars::Visuals::CleanScreenshots.Value)
	{
		return Hook.Original<FN>()(ecx, pView);
	}

	F::Visuals.FOV(pView);

	F::Visuals.ThirdPerson(pView);
}
