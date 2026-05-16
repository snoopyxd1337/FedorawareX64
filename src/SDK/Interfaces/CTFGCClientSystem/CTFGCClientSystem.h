#pragma once
#include "../../Includes/Includes.h"
#include "../CTFPartyClient/CTFPartyClient.h"

namespace S
{
	MAKE_SIGNATURE(CTFGCClientSystem_GetParty, CLIENT_DLL, "48 83 EC ? 48 8B 89 ? ? ? ? 48 85 C9 74 ? BA ? ? ? ? E8 ? ? ? ? 48 85 C0 74 ? 8B 48 ? 85 C9 74 ? 48 8B 40 ? FF C9", 0x0);
	MAKE_SIGNATURE(CTFGCClientSystem_PingThink, CLIENT_DLL, "40 55 41 54 41 55 48 8D AC 24", 0x0);
}

class CTFGCClientSystem
{
public:
	bool Init()
	{
		return true;
	}

	void PreInitGC()
	{
	}

	void PostInit()
	{
	}

	void PostInitGC()
	{
	}

	void ReceivedClientWelcome(const int& msg)
	{
	}

	void Shutdown()
	{
	}

	void Update(float frametime)
	{
	}

	CTFPartyClient* GetParty()
	{
		static auto FN = S::CTFGCClientSystem_GetParty.As<CTFPartyClient*(__thiscall*)(void*)>();
		return FN ? FN(this) : nullptr;
	}

	bool BHaveLiveMatch()
	{
		return false;
	}

	int GetNumMatchInvites()
	{
		return 0;
	}

	bool JoinMMMatch()
	{
		return false;
	}

	bool BConnectedToMatchServer(bool bLiveMatch)
	{
		return false;
	}

	bool BGetLocalPlayerBadgeInfoForTour(int iTourIndex, uint32* pnBadgeLevel, uint32* pnCompletedChallenges)
	{
		return false;
	}

	bool BHasCompetitiveAccess()
	{
		return false;
	}

	bool BIsMatchGroupDisabled(int eMatchGroup)
	{
		return false;
	}

	void ConnectToServer(const char* connect)
	{
	}

	void PingThink()
	{
		static auto FN = S::CTFGCClientSystem_PingThink.As<void(__thiscall*)(void*)>();
		if (FN) { FN(this); }
	}

	bool* PendingPingRefresh()
	{
		return reinterpret_cast<bool*>(reinterpret_cast<uintptr_t>(this) + 1116);
	}

	void DumpPing()
	{
	}

	void FireGameEvent(CGameEvent* pEvent)
	{
	}

	void* GetLobby()
	{
		return nullptr;
	}

	void* GetMatchInvite(void* pUnknown1, void* pUnknown2)
	{
		return nullptr;
	}

	void DumpLobby()
	{
	}

	void DumpParty()
	{
	}
};
