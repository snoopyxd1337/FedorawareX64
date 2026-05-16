#include "../Hooks.h"

MAKE_HOOK(C_BaseEntity_ToolRecordEntities, nullptr, void, __cdecl,
	)
{
	return Hook.Original<FN>()();
}

