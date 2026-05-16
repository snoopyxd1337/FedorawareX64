#include "../Hooks.h"
#include "../../Features/Chams/DMEChams.h"

MAKE_HOOK(CBaseClient_Disconnect, S::CBaseClient_Disconnect(), void, __fastcall, void* ecx, const char* fmt, ...)
{
	F::DMEChams.DeleteMaterials();
	return Hook.Original<FN>()(ecx, fmt);
}
