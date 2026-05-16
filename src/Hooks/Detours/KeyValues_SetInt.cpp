#include "../Hooks.h"

namespace S
{
	MAKE_SIGNATURE(KeyValues_SetInt_Desired, CLIENT_DLL, "48 8B 05 ? ? ? ? 83 78 ? ? 0F 84 ? ? ? ? 48 8B 0D ? ? ? ? 8B D7", 0x0);
	MAKE_SIGNATURE(KeyValues_SetInt_Jump, CLIENT_DLL, "8B E8 E8 ? ? ? ? 3B C7", 0x0);
}

MAKE_HOOK(KeyValues_SetInt, S::KeyValues_SetInt(), void, __fastcall, void* ecx, const char* szKeyName, int iValue)
{
	Hook.Original<FN>()(ecx, szKeyName, iValue);

	static uintptr_t dwDesired = S::KeyValues_SetInt_Desired();
	static uintptr_t dwJump = S::KeyValues_SetInt_Jump();

	/* Scoreboard class reveal */
	if (dwDesired && dwJump && szKeyName && reinterpret_cast<uintptr_t>(_ReturnAddress()) == dwDesired && std::string_view(szKeyName).find("nemesis") != std::string_view::npos)
	{
		*static_cast<uintptr_t*>(_AddressOfReturnAddress()) = dwJump;
	}
}
