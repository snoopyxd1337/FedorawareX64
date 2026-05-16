#include "../Hooks.h"

MAKE_HOOK(CTFPlayerInventory_OnHasNewItems, nullptr, void, __cdecl)
{
	Hook.Original<FN>()();
}
