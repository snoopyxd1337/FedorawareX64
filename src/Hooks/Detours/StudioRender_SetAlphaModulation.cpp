#include "../Hooks.h"

MAKE_HOOK(StudioRender_SetAlphaModulation, Utils::GetVFuncPtr(I::StudioRender, 28), void, __fastcall,
		  void* ecx, float flAlpha)
{
	Hook.Original<FN>()(ecx, G::DrawingStaticProps ? Color::TOFLOAT(Vars::Colours::StaticPropModulation.Value.a) : flAlpha);
}