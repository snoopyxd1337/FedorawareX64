#include "../Hooks.h"

MAKE_HOOK(CTFGCClientSystem_UpdateAssignedLobby, S::CTFGCClientSystem_UpdateAssignedLobby(), bool, __fastcall,
		  void* ecx)
{
	return Hook.Original<FN>()(ecx);
}
