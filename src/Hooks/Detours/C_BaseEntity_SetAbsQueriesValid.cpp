#include "../Hooks.h"

MAKE_HOOK(C_BaseAnimating_SetAbsQueriesValid, nullptr, void, __cdecl,
	bool bValid)
{
	return Hook.Original<FN>()(bValid);
}

