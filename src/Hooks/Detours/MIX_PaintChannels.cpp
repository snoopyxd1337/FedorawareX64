#include "../Hooks.h"

MAKE_HOOK(MIX_PaintChannels, nullptr, void, __fastcall,
	void* ecx, void* edx, int endtime, bool bIsUnderwater)
{
	Hook.Original<FN>()(ecx, edx, endtime, bIsUnderwater);
}
