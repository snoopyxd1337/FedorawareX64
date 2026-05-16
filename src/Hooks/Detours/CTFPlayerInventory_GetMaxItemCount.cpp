#include "../Hooks.h"

MAKE_HOOK(CTFPlayerInventory_GetMaxItemCount, S::CTFPlayerInventory_GetMaxItemCount(), int, __fastcall, void* ecx)
{
	return 3000;
}