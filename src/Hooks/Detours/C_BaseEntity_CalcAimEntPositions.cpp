#include "../Hooks.h"

MAKE_HOOK(C_BaseEntity_CalcAimEntPositions, nullptr, void, __cdecl,
	)
{
	return Hook.Original<FN>()();
}

