#include "../Hooks.h"
#include "../../Features/Chams/DMEChams.h"

MAKE_HOOK(CBaseClient_Connect, S::CBaseClient_Connect(), void, __fastcall,
		  void* ecx, const char* szName, int nUserID, INetChannel* pNetChannel, bool bFakePlayer, int clientChallenge)
{
	F::DMEChams.CreateMaterials();
	return Hook.Original<FN>()(ecx, szName, nUserID, pNetChannel, bFakePlayer, clientChallenge);
}
