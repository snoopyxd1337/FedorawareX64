#pragma once
#include <Windows.h>

static constexpr auto CLIENT_DLL = "client.dll";
static constexpr auto ENGINE_DLL = "engine.dll";
static constexpr auto SERVER_DLL = "server.dll";
static constexpr auto VSTDLIB_DLL = "vstdlib.dll";
static constexpr auto VGUI2_DLL = "vgui2.dll";
static constexpr auto MATSURFACE_DLL = "vguimatsurface.dll";
static constexpr auto MATSYSTEM_DLL = "MaterialSystem.dll";
static constexpr auto STUDIORENDER_DLL = "studiorender.dll";
static constexpr auto TIER0_DLL = "tier0.dll";
static constexpr auto VPHYSICS_DLL = "vphysics.dll";

class CSignature
{
	uintptr_t m_Address = 0;
	LPCSTR m_Name = nullptr;
	LPCSTR m_Module = nullptr;
	LPCSTR m_Pattern = nullptr;
	int m_Offset = 0;

	void Find();

public:
	CSignature(LPCSTR szName, LPCSTR szModule, LPCSTR szPattern, int offset = 0)
		: m_Name(szName), m_Module(szModule), m_Pattern(szPattern), m_Offset(offset) { }

	LPCSTR GetName() const { return m_Name; }
	LPCSTR GetModule() const { return m_Module; }

	// Return the address
	uintptr_t operator()()
	{
		if (m_Address == 0) { Find(); }
		return m_Address;
	}

	template <typename T> T As() { return reinterpret_cast<T>(this->operator()()); }
};

#define MAKE_SIGNATURE(name, module, pattern, offset) inline CSignature name{ #name, module, pattern, offset }

// Signatures
namespace S
{
	// Hooks
	MAKE_SIGNATURE(CBaseAnimating_FrameAdvance, CLIENT_DLL, "48 89 5C 24 ? 48 89 6C 24 ? 57 48 81 EC ? ? ? ? 44 0F 29 54 24", 0x0);
	MAKE_SIGNATURE(CBaseAnimating_Interpolate, CLIENT_DLL, "48 8B C4 48 89 70 ? F3 0F 11 48", 0x0);
	MAKE_SIGNATURE(CBaseAnimating_MaintainSequenceTransitions, CLIENT_DLL, "4C 89 4C 24 ? 41 56", 0x0);
	MAKE_SIGNATURE(CBaseAnimating_UpdateClientSideAnimation, CLIENT_DLL, "48 89 5C 24 ? 57 48 83 EC ? 48 8B D9 E8 ? ? ? ? 48 8B F8 48 85 C0 74 ? 48 8B 00 48 8B CF FF 90 ? ? ? ? 84 C0 75 ? 33 FF 48 3B DF", 0x0);
	MAKE_SIGNATURE(CBaseCombatWeapon_AddToCritBucket, CLIENT_DLL, "48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC ? F3 0F 10 0D", 0x0);
	MAKE_SIGNATURE(CBaseCombatWeapon_IsAllowedToWithdrawFromCritBucket, CLIENT_DLL, "40 53 48 83 EC ? 48 8B D9 0F B7 81", 0x0);
	MAKE_SIGNATURE(CBaseEntity_FireBullets, CLIENT_DLL, "48 8B C4 48 89 58 ? 48 89 68 ? 48 89 70 ? 48 89 78 ? 41 56 48 81 EC ? ? ? ? 48 8B DA", 0x0);
	MAKE_SIGNATURE(CBaseEntity_Interpolate, CLIENT_DLL, "48 8B C4 48 89 58 ? 48 89 70 ? 48 89 78 ? 55 41 54 41 55 41 56 41 57 48 8D 68 ? 48 81 EC", 0x0);
	MAKE_SIGNATURE(CBaseEntity_SetAbsVelocity, CLIENT_DLL, "48 89 5C 24 ? 57 48 83 EC ? F3 0F 10 81 ? ? ? ? 48 8B DA 0F 2E 02", 0x0);
	MAKE_SIGNATURE(C_BaseEntity_AddVar, CLIENT_DLL, "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 8B F9 48 8B DA 33 F6", 0x0);
	MAKE_SIGNATURE(C_BaseEntity_BaseInterpolatePart1, CLIENT_DLL, "48 89 5C 24 ? 56 57 41 55 41 56 41 57 48 83 EC ? 4C 8B BC 24", 0x0);
	MAKE_SIGNATURE(C_BaseEntity_InterpolateServerEntities, CLIENT_DLL, "48 8B C4 48 89 58 ? 48 89 70 ? 48 89 78 ? 55 41 54 41 55 41 56 41 57 48 8D 68 ? 48 81 EC ? ? ? ? 48 8B 05", 0x0);
	MAKE_SIGNATURE(C_BaseEntity_ResetLatched, CLIENT_DLL, "40 56 48 83 EC ? 48 8B 01 48 8B F1 FF 90 ? ? ? ? 84 C0 75", 0x0);
	MAKE_SIGNATURE(CBasePlayer_CalcViewModelView, CLIENT_DLL, "48 89 74 24 ? 55 41 56 41 57 48 8D AC 24", 0x0);
	MAKE_SIGNATURE(CBaseViewModel_ShouldFlipViewModel, CLIENT_DLL, "40 57 48 83 EC ? 8B 91 ? ? ? ? 85 D2", 0x0);
	MAKE_SIGNATURE(COP_RenderSprites_RenderSpriteCard, CLIENT_DLL, "48 8B C4 48 89 58 ? 57 41 54", 0x0);
	MAKE_SIGNATURE(CPrediction_RunSimulation, CLIENT_DLL, "48 83 EC 38 4C 8B 44", 0x0);
	MAKE_SIGNATURE(CTFPlayer_AvoidPlayers, CLIENT_DLL, "48 89 54 24 ? 55 41 56", 0x0);
	MAKE_SIGNATURE(CTFPlayer_FireEvent, CLIENT_DLL, "48 8B C4 48 89 58 ? 48 89 70 ? 48 89 78 ? 55 41 54 41 56 41 57 48 8D 68 ? 48 81 EC ? ? ? ? 48 8B F9", 0x0);
	MAKE_SIGNATURE(CTFRagdoll_CreateTFRagdoll, CLIENT_DLL, "48 89 4C 24 ? 55 53 56 57 48 8D AC 24 ? ? ? ? B8 ? ? ? ? E8 ? ? ? ? 48 2B E0 8B 91", 0x0);
	MAKE_SIGNATURE(CTFWeaponBase_CalcIsAttackCritical, CLIENT_DLL, "48 89 74 24 ? 57 48 83 EC ? 48 8B F9 E8 ? ? ? ? 48 8B C8 C7 44 24 ? ? ? ? ? 4C 8D 0D ? ? ? ? 33 D2 4C 8D 05 ? ? ? ? E8 ? ? ? ? 48 8B F0 48 85 C0 0F 84 ? ? ? ? 48 8B 10", 0x0);
	MAKE_SIGNATURE(CTFWeaponBase_UpdateAllViewmodelAddons, CLIENT_DLL, "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 8B D9 E8 ? ? ? ? 48 8B F8", 0x0);
	MAKE_SIGNATURE(CAchievementMgr_CheckAchievementsEnabled, CLIENT_DLL, "40 53 48 83 EC ? 48 8B 05 ? ? ? ? 48 8B D9 48 8B 48 ? 48 85 C9 0F 84", 0x0);
	MAKE_SIGNATURE(CBaseClient_Connect, ENGINE_DLL, "48 8B C4 48 89 58 ? 48 89 70 ? 48 89 78 ? 55 41 54 41 56 41 57 48 8D 68 ? 48 81 EC ? ? ? ? 48 8B F1", 0x0);
	MAKE_SIGNATURE(CBaseClient_Disconnect, ENGINE_DLL, "48 8B C4 48 89 58 ? 48 89 70 ? 48 89 78 ? 55 41 54 41 56 41 57 48 8D 68 ? 48 81 EC ? ? ? ? 48 8B F1 48 8B E9", 0x0);
	MAKE_SIGNATURE(CBaseClient_SendSignonData, ENGINE_DLL, "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 8B F1 48 8D 15", 0x0);
	MAKE_SIGNATURE(CEconItemSchema_GetItemDefinition, CLIENT_DLL, "40 53 48 83 EC ? 48 8B D9 8B 02", 0x0);
	MAKE_SIGNATURE(CEconNotification_HasNewItems, CLIENT_DLL, "48 89 5C 24 ? 57 48 83 EC ? 48 8B F1 48 8D 15", 0x0);
	MAKE_SIGNATURE(CheckSimpleMaterial, ENGINE_DLL, "40 56 48 83 EC ? 48 8B F1 48 85 C9 75 ? 32 C0", 0x0);
	MAKE_SIGNATURE(CHudCrosshair_GetDrawPosition, CLIENT_DLL, "48 8B C4 55 53 56 41 54 41 55", 0x0);
	MAKE_SIGNATURE(CInterpolatedVarArrayBase_Interpolate, CLIENT_DLL, "48 8B C4 48 89 58 ? 48 89 70 ? 48 89 78 ? 55 41 54 41 56 41 57 48 8D 68 ? 48 81 EC ? ? ? ? 0F 28 05", 0x0);
	MAKE_SIGNATURE(CInventoryManager_ShowItemsPickedUp, CLIENT_DLL, "44 88 4C 24 ? 44 88 44 24 ? 53 41 56", 0x0);
	MAKE_SIGNATURE(CL_LoadWhitelist, ENGINE_DLL, "40 56 48 83 EC ? 83 3D ? ? ? ? ? 48 8B F1 0F 8E", 0x0);
	MAKE_SIGNATURE(CL_Move, ENGINE_DLL, "40 55 53 48 8D AC 24 ? ? ? ? B8 ? ? ? ? E8 ? ? ? ? 48 2B E0 83 3D", 0x0);
	MAKE_SIGNATURE(CL_SendMove, ENGINE_DLL, "48 8B C4 48 89 58 ? 48 89 70 ? 48 89 78 ? 55 41 54 41 56 41 57 48 8D 68 ? 48 81 EC ? ? ? ? 48 8B 05", 0x0);
	MAKE_SIGNATURE(CL_ReadPackets, ENGINE_DLL, "4C 8B DC 49 89 5B ? 55 56 57 41 54 41 55 41 56 41 57 48 83 EC ? 48 8B 05", 0x0);
	MAKE_SIGNATURE(ClientModeTFNormal_UpdateSteamRichPresence, CLIENT_DLL, "48 8B C4 48 89 58 ? 48 89 70 ? 48 89 78 ? 55 41 54 41 56 41 57 48 8D 68 ? 48 81 EC ? ? ? ? 48 8B 15", 0x0);
	MAKE_SIGNATURE(ClientState_GetClientInterpAmount, ENGINE_DLL, "48 83 EC ? 48 8B 0D ? ? ? ? 48 85 C9 75", 0x0);
	MAKE_SIGNATURE(ClientState_ProcessFixAngle, ENGINE_DLL, "40 53 48 83 EC ? F3 0F 10 42", 0x0);
	MAKE_SIGNATURE(CMatchInviteNotification_OnTick, CLIENT_DLL, "40 53 48 83 EC ? 48 8B D9 E8 ? ? ? ? F7 83", 0x0);
	MAKE_SIGNATURE(CNetChan_SendNetMsg, ENGINE_DLL, "48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 41 56 48 83 EC ? 48 8B F1 45 0F B6 F1", 0x0);
	MAKE_SIGNATURE(CNewParticleEffect_DrawModel, CLIENT_DLL, "48 8B C4 48 89 58 ? 48 89 70 ? 48 89 78 ? 55 41 54 41 56 41 57 48 8D 68 ? 48 81 EC ? ? ? ? 48 8B D9", 0x0);
	MAKE_SIGNATURE(COcclusionSystem_IsOccluded, ENGINE_DLL, "48 8B C4 48 89 58 ? 48 89 70 ? 48 89 78 ? 55 41 54 41 56 41 57 48 8D 68 ? 48 81 EC ? ? ? ? 48 8B 05", 0x0);
	MAKE_SIGNATURE(CRendering3dView_EnableWorldFog, CLIENT_DLL, "40 53 48 83 EC ? 48 8B 0D ? ? ? ? 48 89 74 24", 0x0);
	MAKE_SIGNATURE(CSequenceTransitioner_CheckForSequenceChange, CLIENT_DLL, "48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 8B F9 48 85 D2 74", 0x0);
	MAKE_SIGNATURE(CSkyboxView_Enable3dSkyboxFog, CLIENT_DLL, "40 57 48 83 EC ? E8 ? ? ? ? 48 8B F8 48 85 C0 0F 84 ? ? ? ? 48 8B 0D", 0x0);
	MAKE_SIGNATURE(CSoundEmitterSystem_EmitSound, CLIENT_DLL, "48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 41 56 48 81 EC ? ? ? ? 49 8B D9", 0x0);
	MAKE_SIGNATURE(CStaticPropMgr_ComputePropOpacity, ENGINE_DLL, "48 89 5C 24 ? 48 89 7C 24 ? 41 54 41 56 41 57 48 83 EC ? 48 8B 05", 0x0);
	MAKE_SIGNATURE(CStaticPropMgr_DrawStaticProps, ENGINE_DLL, "4C 8B DC 49 89 5B ? 49 89 6B ? 49 89 73 ? 57 41 54 41 55 41 56 41 57 48 83 EC ? 4C 8B 3D", 0x0);
	MAKE_SIGNATURE(CTFGameRules_ModifySentChat, CLIENT_DLL, "48 89 5C 24 ? 57 48 83 EC ? 48 8B FA 48 8B D9 48 85 D2 74", 0x0);
	MAKE_SIGNATURE(CTFGCClientSystem_UpdateAssignedLobby, CLIENT_DLL, "40 55 53 41 54 41 56 41 57 48 8B EC", 0x0);
	MAKE_SIGNATURE(CTFPartyClient_BCanRequestToJoinPlayer, CLIENT_DLL, "48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 8B F1 4C 8B 41", 0x0);
	MAKE_SIGNATURE(CTFPlayerInventory_GetMaxItemCount, CLIENT_DLL, "40 53 48 83 EC ? 48 8B 89 ? ? ? ? BB", 0x0);
	MAKE_SIGNATURE(CTFPlayerShared_InCond, CLIENT_DLL, "48 89 5C 24 ? 57 48 83 EC ? 8B DA 48 8B F9 83 FA ? 7D", 0x0);
	MAKE_SIGNATURE(CTFPlayerShared_IsPlayerDominated, CLIENT_DLL, "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 63 F2 48 8B D9 E8", 0x0);
	MAKE_SIGNATURE(CViewRender_DrawUnderwaterOverlay, CLIENT_DLL, "4C 8B DC 41 56 48 81 EC ? ? ? ? 4C 8B B1", 0x0);
	MAKE_SIGNATURE(DataTable_Warning, ENGINE_DLL, "48 89 4C 24 ? 48 89 54 24 ? 4C 89 44 24 ? 4C 89 4C 24 ? B8 ? ? ? ? E8 ? ? ? ? 48 2B E0 4C 8B C1 4C 8D 8C 24 ? ? ? ? 48 8D 4C 24 ? 8D 50", 0x0);
	MAKE_SIGNATURE(DoEnginePostProcessing, CLIENT_DLL, "48 8B C4 44 89 48 ? 44 89 40 ? 89 50 ? 89 48", 0x0);
	MAKE_SIGNATURE(DSP_Process, ENGINE_DLL, "48 89 5C 24 ? 55 41 54 41 57 48 83 EC ? 48 63 D9", 0x0);
	MAKE_SIGNATURE(FX_FireBullets, CLIENT_DLL, "48 89 5C 24 ? 48 89 74 24 ? 4C 89 4C 24 ? 55", 0x0);
	MAKE_SIGNATURE(GetClientInterpAmount, CLIENT_DLL, "40 53 48 83 EC ? 8B 05 ? ? ? ? A8 ? 75 ? 48 8B 0D ? ? ? ? 48 8D 15", 0x0);
	MAKE_SIGNATURE(IsLocalPlayerUsingVisionFilterFlags, CLIENT_DLL, "40 53 48 83 EC ? 44 0F B6 C2 48 8B 05", 0x0);
	MAKE_SIGNATURE(KeyValues_SetInt, CLIENT_DLL, "40 53 48 83 EC ? 41 8B D8 41 B0", 0x0);
	MAKE_SIGNATURE(NetChannel_SendDatagram, ENGINE_DLL, "40 55 57 41 56 48 8D AC 24", 0x0);
	MAKE_SIGNATURE(NotificationQueue_Add, CLIENT_DLL, "48 89 5C 24 ? 57 48 83 EC ? 48 8B F9 48 8B 0D ? ? ? ? 48 8B 01 FF 90 ? ? ? ? 84 C0 75", 0x0);
	MAKE_SIGNATURE(S_StartDynamicSound, ENGINE_DLL, "4C 8B DC 57 48 81 EC", 0x0);
	MAKE_SIGNATURE(UTIL_TraceLine, CLIENT_DLL, "48 8B C4 48 89 58 ? 48 89 70 ? 48 89 78 ? 55 41 54 41 55 41 56 41 57 48 8D 68 ? 48 81 EC ? ? ? ? 0F 29 70", 0x0);
	MAKE_SIGNATURE(ViewRender_PerformScreenSpaceEffects, CLIENT_DLL, "48 8B C4 48 89 58 ? 48 89 70 ? 48 89 78 ? 55 41 54 41 56 41 57 48 8D 68 ? 48 81 EC ? ? ? ? 48 8B 0D", 0x0);
	MAKE_SIGNATURE(CMaterial_Uncache, MATSYSTEM_DLL, "48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 81 EC ? ? ? ? 48 8B F9", 0x0);
	MAKE_SIGNATURE(CTFMatchSummary_OnTick, CLIENT_DLL, "40 53 48 83 EC ? 48 8B D9 E8 ? ? ? ? 48 8B 0D ? ? ? ? 48 85 C9 74", 0x0);

	// Functions
	MAKE_SIGNATURE(CMatchInviteNotification_AcceptMatch, CLIENT_DLL, "48 89 5C 24 ? 57 48 83 EC ? 48 8B F9 E8 ? ? ? ? 48 8B D8 48 85 C0 74 ? 48 8B 00", 0x0);
	MAKE_SIGNATURE(CTFKnife_IsBehindAndFacingTarget, CLIENT_DLL, "48 8B C4 48 89 58 ? 48 89 70 ? 48 89 78 ? 55 41 54 41 55 41 56 41 57 48 8D 68 ? 48 81 EC ? ? ? ? 0F 29 70", 0x0);

	// Values
	MAKE_SIGNATURE(RandomSeed, CLIENT_DLL, "0F B6 1D ? ? ? ? 89 9D", 0x0);
	MAKE_SIGNATURE(AllowSecureServers, ENGINE_DLL, "40 53 48 83 EC ? 48 8B D9 80 3D", 0x0);
}
