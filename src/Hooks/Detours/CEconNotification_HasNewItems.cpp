#include "../Hooks.h"
#include "../../Features/Items/Items.h"

MAKE_HOOK(CEconNotification_HasNewItems, S::CEconNotification_HasNewItems(), CEconNotification*, __fastcall,
		  void* ecx)
{
	auto* pNotification = Hook.Original<FN>()(ecx);
	F::Items.AddNotifcation(pNotification);
	return pNotification;
}
