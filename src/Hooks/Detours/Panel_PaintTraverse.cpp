#include "../Hooks.h"

#include "../../Features/Visuals/Visuals.h"
#include "../../Features/Camera/CameraWindow.h"

MAKE_HOOK(Panel_PaintTraverse, Utils::GetVFuncPtr(I::VGuiPanel, 41), void, __fastcall,
		  void* ecx, unsigned int vgui_panel, bool force_repaint, bool allow_force)
{
	if (I::EngineClient->IsTakingScreenshot() && Vars::Visuals::CleanScreenshots.Value)
	{
		return Hook.Original<FN>()(ecx, vgui_panel, force_repaint, allow_force);
	}
	F::CameraWindow.Draw();
	//if (F::Visuals.RemoveScope(vgui_panel)) { return; }

	Hook.Original<FN>()(ecx, vgui_panel, force_repaint, allow_force);
}
