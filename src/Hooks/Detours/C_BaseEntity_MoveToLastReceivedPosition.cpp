#include "../Hooks.h"

MAKE_HOOK(C_BaseEntity_MoveToLastReceivedPosition, nullptr, void, __fastcall,
	void* ecx, void* edx, bool force)
{
	return Hook.Original<FN>()(ecx, edx, force);
}

