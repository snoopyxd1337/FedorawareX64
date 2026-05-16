#include "../Hooks.h"

MAKE_HOOK(CInventoryManager_ShowItemsPickedUp, S::CInventoryManager_ShowItemsPickedUp(), bool, __fastcall,
		  void* ecx, bool bForce, bool bReturnToGame, bool bNoPanel)
{
	Hook.Original<FN>()(ecx, true, true, true);
	return false;
}
