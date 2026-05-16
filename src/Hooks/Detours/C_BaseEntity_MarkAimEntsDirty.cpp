#include "../Hooks.h"

MAKE_HOOK(C_BaseEntity_MarkAimEntsDirty, nullptr, void, __cdecl,
	)
{
	return Hook.Original<FN>()();
}

