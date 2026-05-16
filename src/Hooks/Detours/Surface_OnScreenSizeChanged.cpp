#include "../Hooks.h"

MAKE_HOOK(Surface_OnScreenSizeChanged, Utils::GetVFuncPtr(I::VGuiSurface, 111), void, __fastcall,
		  void* ecx, int nOldWidth, int nOldHeight)
{
	Hook.Original<FN>()(ecx, nOldWidth, nOldHeight);

	g_ScreenSize.Update();
	g_Draw.ReloadFonts();
}