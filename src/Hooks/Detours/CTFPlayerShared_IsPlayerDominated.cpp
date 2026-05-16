#include "../Hooks.h"

namespace S
{
	MAKE_SIGNATURE(CTFPlayerShared_IsPlayerDominated_Desired, CLIENT_DLL, "84 C0 74 ? 45 84 FF 74", 0x0);
	MAKE_SIGNATURE(CTFPlayerShared_IsPlayerDominated_Jump, CLIENT_DLL, "8B E8 E8 ? ? ? ? 3B C7", 0x0);
}

MAKE_HOOK(CTFPlayerShared_IsPlayerDominated, S::CTFPlayerShared_IsPlayerDominated(), bool, __fastcall, void* ecx, int index)
{
	const bool bResult = Hook.Original<FN>()(ecx, index);

	if (!bResult)
	{
		static uintptr_t dwDesired = S::CTFPlayerShared_IsPlayerDominated_Desired();
		static uintptr_t dwJump = S::CTFPlayerShared_IsPlayerDominated_Jump();

		if (dwDesired && dwJump && reinterpret_cast<uintptr_t>(_ReturnAddress()) == dwDesired)
		{
			*static_cast<uintptr_t*>(_AddressOfReturnAddress()) = dwJump;
		}
	}

	return bResult;
}
