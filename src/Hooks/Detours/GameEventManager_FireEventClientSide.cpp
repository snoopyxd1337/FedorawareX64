#include "../Hooks.h"

#include "../../Features/Killstreak/Killstreak.h"
#include "../../Features/Fedworking/Fedworking.h"
#include "../../Utils/FunctionalDebug/FunctionalDebug.h"

MAKE_HOOK(GameEventManager_FireEventClientSide, Utils::GetVFuncPtr(I::GameEventManager, 8), bool, __fastcall,
		  void* ecx, CGameEvent* pEvent)
{
	FDBG_SCOPE("Hook.GameEventManager_FireEventClientSide");

	if (!ecx || !pEvent) { FDBG_SKIP("Hook.GameEventManager_FireEventClientSide", "ecx or event null"); return false; }

	const auto eventName = pEvent->GetName();
	if (!eventName) { FDBG_SKIP("GameEvent.Features", "event name null"); return Hook.Original<FN>()(ecx, pEvent); }

	const FNV1A_t uNameHash = FNV1A::Hash(eventName);
	FDBG_CALL("Feature.Killstreaker.FireEvents", F::Killstreaker.FireEvents(pEvent, uNameHash));

	if (uNameHash == FNV1A::HashConst("party_chat"))
	{
		const auto msg = pEvent->GetString("text");

		// Handle networking
		if (Utils::StartsWith(msg, "FED@"))
		{
			if (Vars::Misc::PartyNetworking.Value)
			{
				FDBG_CALL("Feature.Fedworking.HandleMessage", F::Fedworking.HandleMessage(msg));
			}
			else
			{
				FDBG_SKIP("Feature.Fedworking.HandleMessage", "PartyNetworking disabled");
			}
			return false;
		}
	}

	bool result = false;
	FDBG_CALL("GameEvent.Original", result = Hook.Original<FN>()(ecx, pEvent));
	return result;
}
