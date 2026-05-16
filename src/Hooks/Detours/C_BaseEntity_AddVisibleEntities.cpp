#include "../Hooks.h"

MAKE_HOOK(C_BaseEntity_AddVisibleEntities, nullptr, void, __cdecl,
	)
{
	return Hook.Original<FN>()();
}

