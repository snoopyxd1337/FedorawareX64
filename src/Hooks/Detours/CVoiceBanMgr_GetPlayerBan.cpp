#include "../Hooks.h"

MAKE_HOOK(CVoiceBanMgr_GetPlayerBan, nullptr, bool, __fastcall,
	void* ecx, void* edx, char const playerID[SIGNED_GUID_LEN])
{
	const bool bReturn = Hook.Original<FN>()(ecx, edx, playerID);
	//Utils::ConLog("CVoiceBanMgr_GetPlayerBan", tfm::format("%s is%s voice-banned.", playerID, (bReturn ? "" : " not")).c_str(), {118, 116, 107, 255});
	return bReturn;
}

