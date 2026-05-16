#include "../Hooks.h"

#include "../../Features/Misc/Misc.h"

MAKE_HOOK(BaseClientDLL_PostEntity, Utils::GetVFuncPtr(I::BaseClientDLL, 6), void, __fastcall,
		  void* ecx)
{
	Hook.Original<FN>()(ecx);
	I::EngineClient->ClientCmd_Unrestricted("r_maxdlights 69420");
	I::EngineClient->ClientCmd_Unrestricted("r_dynamic 1");
}
