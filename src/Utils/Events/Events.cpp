#include "Events.h"
#include "../../Features/ChatInfo/ChatInfo.h"
#include "../../Features/Resolver/Resolver.h"
#include "../../Features/AntiHack/CheaterDetection/CheaterDetection.h"
#include "../../Features/Visuals/Visuals.h"
#include "../../Features/Killstreak/Killstreak.h"
#include "../../Features/Backtrack/Backtrack.h"
#include "../../Features/Misc/Misc.h"
#include "../../Features/Killsay/Killsay.h"
#include "../../Features/CritHack/CritHack.h"
#include "../Minidump/Minidump.h"
#include "../FunctionalDebug/FunctionalDebug.h"

// TODO: Add listener to all events
void CEventListener::Setup(const std::deque<const char*>& deqEvents)
{
	if (deqEvents.empty())
		return;

	for (auto szEvent : deqEvents)
	{
		I::GameEventManager->AddListener(this, szEvent, false);

		if (!I::GameEventManager->FindListener(this, szEvent))
		{
#if defined(FW_DEBUG_DIAGNOSTICS)
			OutputDebugStringA(std::format("failed to add listener: {}\n", szEvent).c_str());
#endif
			FW_TRACE(std::format("[ERROR] Failed to add game event listener: {}", szEvent).c_str());
		}
	}
}

void CEventListener::Destroy()
{
	I::GameEventManager->RemoveListener(this);
}

void CEventListener::FireGameEvent(CGameEvent* pEvent)
{
	FDBG_SCOPE("EventListener.FireGameEvent");

	if (pEvent == nullptr) { FDBG_SKIP("EventListener.FireGameEvent", "event null"); return; }
	if (I::EngineClient->IsPlayingTimeDemo()) { FDBG_SKIP("EventListener.FireGameEvent", "playing timedemo"); return; }

	const FNV1A_t uNameHash = FNV1A::Hash(pEvent->GetName());
	FDBG_CALL("Feature.ChatInfo.Event", F::ChatInfo.Event(pEvent, uNameHash));
	//F::Statistics.Event(pEvent, uNameHash);
	// Lol
	FDBG_CALL("Feature.Killsay.FireGameEvent", F::Killsay.FireGameEvent(uNameHash, pEvent));
	FDBG_CALL("Feature.CritHack.Event", F::CritHack.Event(pEvent, uNameHash));
	/*F::Killstreaker.FireEvents(pEvent, uNameHash);*/

	if (uNameHash == FNV1A::HashConst("player_hurt"))
	{
		FDBG_CALL("Feature.Resolver.OnPlayerHurt", F::Resolver.OnPlayerHurt(pEvent));
		FDBG_CALL("Feature.Backtrack.PlayerHurt", F::Backtrack.PlayerHurt(pEvent));
		FDBG_CALL("Feature.BadActors.ReportDamage", F::BadActors.ReportDamage(pEvent));
	}

	// Pickup Timers
	if (Vars::Visuals::PickupTimers.Value && uNameHash == FNV1A::HashConst("item_pickup"))
	{
		const auto itemName = pEvent->GetString("item");
		if (const auto& pEntity = I::ClientEntityList->GetClientEntity(I::EngineClient->GetPlayerForUserID(pEvent->GetInt("userid"))))
		{
			if (std::strstr(itemName, "medkit"))
			{
				F::Visuals.PickupDatas.push_back({ 1, I::EngineClient->Time(), pEntity->GetAbsOrigin() });
			}
			else if (std::strstr(itemName, "ammopack"))
			{
				F::Visuals.PickupDatas.push_back({ 0, I::EngineClient->Time(), pEntity->GetAbsOrigin() });
			}
		}
	}
}
