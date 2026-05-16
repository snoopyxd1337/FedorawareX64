#include "../Hooks.h"

MAKE_HOOK(CTFPartyClient_OnInQueueChanged, nullptr, void, __fastcall,
		  void* ecx, void* edx, int iMatchGroup)
{
	Hook.Original<FN>()(ecx, edx, iMatchGroup);
}
