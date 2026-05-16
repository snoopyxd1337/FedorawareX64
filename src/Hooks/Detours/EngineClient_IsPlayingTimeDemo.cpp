#include "../Hooks.h"

MAKE_HOOK(EngineClient_IsPlayingTimeDemo, Utils::GetVFuncPtr(I::EngineClient, 78), bool, __fastcall,
		  void* ecx)
{
	return Hook.Original<FN>()(ecx);
}
