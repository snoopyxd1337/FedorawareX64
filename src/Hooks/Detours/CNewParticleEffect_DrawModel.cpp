#include "../Hooks.h"

MAKE_HOOK(CNewParticleEffect_DrawModel, S::CNewParticleEffect_DrawModel(), int, __fastcall, void* ecx, int flags)
{
	if (!Vars::Visuals::ParticlesIgnoreZ.Value)
	{
		return Hook.Original<FN>()(ecx, flags);
	}

	if (const auto& pRC = I::MaterialSystem->GetRenderContext())
	{
		pRC->DepthRange(0.0f, 0.02f);
		const int nReturn = Hook.Original<FN>()(ecx, flags);
		pRC->DepthRange(0.0f, 1.0f);
		return nReturn;
	}

	return Hook.Original<FN>()(ecx, flags);	//	warning
}
