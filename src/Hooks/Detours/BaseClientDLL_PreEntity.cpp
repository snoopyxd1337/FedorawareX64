#include "../Hooks.h"

MAKE_HOOK(BaseClientDLL_PreEntity, Utils::GetVFuncPtr(I::BaseClientDLL, 5), void, __fastcall,
		  void* ecx, const char* szMapName)
{
	Hook.Original<FN>()(ecx, szMapName);
}
