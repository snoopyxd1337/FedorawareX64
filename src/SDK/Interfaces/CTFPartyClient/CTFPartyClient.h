#pragma once
#include "../../Includes/Includes.h"

namespace S
{
	MAKE_SIGNATURE(CTFPartyClient_LoadSavedCasualCriteria, CLIENT_DLL, "48 83 79 ? ? C6 81 ? ? ? ? ? 74 ? 80 79 ? ? 74 ? C6 81 ? ? ? ? ? 48 8B 15", 0x0);
	MAKE_SIGNATURE(CTFPartyClient_BInQueueForMatchGroup, CLIENT_DLL, "48 89 5C 24 ? 57 48 83 EC ? 48 8B F9 8B DA 8B CA E8 ? ? ? ? 84 C0", 0x0);
	MAKE_SIGNATURE(CTFPartyClient_RequestQueueForMatch, CLIENT_DLL, "40 55 56 48 81 EC ? ? ? ? 48 63 F2", 0x0);
}

class CTFPartyClient
{
public:
	void LoadSavedCasualCriteria()
	{
		static auto FN = S::CTFPartyClient_LoadSavedCasualCriteria.As<void(__thiscall*)(void*)>();
		if (FN) { FN(this); }
	}

	bool BInStandbyQueue()
	{
		return BInQueueForMatchGroup(k_eTFMatchGroup_Casual_Default);
	}

	//CTFPartyClient::BCanQueueForMatch(ETFMatchGroup, CUtlVector<CTFPartyClient::QueueEligibilityData_t, CUtlMemory<CTFPartyClient::QueueEligibilityData_t, int>>&)
	bool BCanQueueForMatch(int eMatchGroup, void* vecQueueEligibilityData)
	{
		return !BInQueueForMatchGroup(eMatchGroup);
	}

	bool BCanQueueForStandby()
	{
		return false;
	}

	bool BInQueueForMatchGroup(int eMatchGroup)
	{
		static auto FN = S::CTFPartyClient_BInQueueForMatchGroup.As<bool(__thiscall*)(void*, int)>();
		return FN ? FN(this, eMatchGroup) : false;
	}

	//CTFPartyOptions&
	bool BMakeUpdateMsg(void* pPartyOptions)
	{
		return false;
	}

	void CancelOutgoingJoinRequestOrIncomingInvite(CSteamID steamID)
	{
	}

	void CheckResetSentOptions()
	{
	}

	void OnInQueueChanged()
	{
	}

	void RequestQueueForStandby()
	{
		RequestQueueForMatch(k_eTFMatchGroup_Casual_Default);
	}

	bool UpdateActiveParty()
	{
		return false;
	}

	void RequestQueueForMatch(int eMatchGroup)
	{
		static auto FN = S::CTFPartyClient_RequestQueueForMatch.As<void(__thiscall*)(void*, int)>();
		if (FN) { FN(this, eMatchGroup); }
	}
};
