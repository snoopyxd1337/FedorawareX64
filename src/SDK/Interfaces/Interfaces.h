#pragma once

#ifndef abstract_class
#define abstract_class class __declspec(novtable)
#endif

#include "EngineClient/EngineClient.h"
#include "EngineEffects/EngineEffects.h"
#include "BaseClientDLL/BaseClientDLL.h"
#include "ClientDLLSharedAppSystems/ClientDLLSharedAppSystems.h"
#include "ClientModeShared/ClientModeShared.h"
#include "GlobalVarsBase/GlobalVarsBase.h"
#include "ClientEntityList/ClientEntityList.h"
#include "ModelInfo/ModelInfo.h"
#include "DebugOverlay/DebugOverlay.h"
#include "EngineTrace/EngineTrace.h"
#include "GameMovement/GameMovement.h"
#include "VPhysics/VPhysics.h"
#include "MoveHelper/MoveHelper.h"
#include "ConVar/ConVar.h"
#include "Prediction/Prediction.h"
#include "Panel/Panel.h"
#include "Surface/Surface.h"
#include "EngineVGui/EngineVGui.h"
#include "NetChannel/NetChannel.h"
#include "GameEvent/GameEvent.h"
#include "Material/Material.h"
#include "ModelRender/ModelRender.h"
#include "ViewRender/ViewRender.h"
#include "RenderView/RenderView.h"
#include "Input/Input.h"
#include "KeyValuesSystem/KeyValuesSystem.h"
#include "UniformRandomStream/UniformRandomStream.h"
#include "InputSystem/InputSystem.h"
#include "EffectsClient/EffectsClient.h"
#include "AchievementMgr/AchievementMgr.h"
#include "ViewRenderBeams/ViewRenderBeams.h"
#include "EngineSound/EngineSound.h"
#include "CTFGameRules/CTFGameRules.h"
#include "CTFGCClientSystem/CTFGCClientSystem.h"
#include "CTFPartyClient/CTFPartyClient.h"
#include "CTFInventoryManager/CTFInventoryManager.h"
#include "Localize/Localize.h"
#include "CCommonHostState/CCommonHostState.h"
#include "ICenterPrint/ICenterPrint.h"
#include "IOcclusionSystem/IOcclusionSystem.h"
#include "CTFGameMovement/CTFGameMovement.h"

#include <d3d9.h>

class CThirdPersonManager
{
public:
	void	SetCameraOffsetAngles(const Vector& vecOffset) { m_vecCameraOffset = vecOffset; }
	const Vector& GetCameraOffsetAngles(void) const { return m_vecCameraOffset; }

	void	SetDesiredCameraOffset(const Vector& vecOffset) { m_vecDesiredCameraOffset = vecOffset; }
	const Vector& GetDesiredCameraOffset(void) const { return m_vecDesiredCameraOffset; }

	// Vector GetFinalCameraOffset(void)
	// {
	// 	Vector vDesired = GetDesiredCameraOffset();

	// 	if (m_flUpFraction != 1.0f)
	// 	{
	// 		vDesired.z += m_flUpOffset;
	// 	}

	// 	return vDesired;
	// }

	void	SetCameraOrigin(const Vector& vecOffset) { m_vecCameraOrigin = vecOffset; }
	const Vector& GetCameraOrigin(void) const { return m_vecCameraOrigin; }

	/*void	Update(void);*/

	//void	PositionCamera(CBaseEntity* pPlayer, const QAngle& angles);

	void	UseCameraOffsets(bool bUse) { m_bUseCameraOffsets = bUse; }
	bool	UsingCameraOffsets(void) { return m_bUseCameraOffsets; }

	const QAngle& GetCameraViewAngles(void) const { return m_ViewAngles; }

	/*Vector	GetDistanceFraction(void);

	bool	WantToUseGameThirdPerson(void);*/

	void	SetOverridingThirdPerson(bool bOverride) { m_bOverrideThirdPerson = bOverride; }
	bool	IsOverridingThirdPerson(void) { return m_bOverrideThirdPerson; }

	/*void	Init(void);*/

	void	SetForcedThirdPerson(bool bForced) { m_bForced = bForced; }
	bool	GetForcedThirdPerson() const { return m_bForced; }
public:
	Vector		m_vecCameraOffset;
	// Distances from the center
	Vector		m_vecDesiredCameraOffset;

	Vector m_vecCameraOrigin;

	bool	m_bUseCameraOffsets;

	QAngle	m_ViewAngles;

	float	m_flFraction;
	float	m_flUpFraction;

	float	m_flTargetFraction;
	float	m_flTargetUpFraction;

	bool	m_bOverrideThirdPerson;

	bool	m_bForced;

	float	m_flUpOffset;

	float	m_flLerpTime;
	float	m_flUpLerpTime;
};

class CClockDriftMgr
{
private:
	enum
	{
		// This controls how much it smoothes out the samples from the server.
		NUM_CLOCKDRIFT_SAMPLES = 16
	};
public:
	// This holds how many ticks the client is ahead each time we get a server tick.
	// We average these together to get our estimate of how far ahead we are.
	float m_ClockOffsets[NUM_CLOCKDRIFT_SAMPLES];
	int m_iCurClockOffset;

	int m_nServerTick;		// Last-received tick from the server.
	int	m_nClientTick;		// The client's own tick counter (specifically, for interpolation during rendering).
							// The server may be on a slightly different tick and the client will drift towards it.
};

class CClientState
{
public:
	byte pad0[24];
	int m_Socket;
	INetChannel* m_NetChannel;
	unsigned int m_ChallengeNr;
	double m_flConnectTime;
	int m_nRetryNumber;
	char m_szRetryAddress[260];
	byte pad_sretry[16]; // CUtlString
	int m_retryChallenge;
	int m_nSignonState;
	double m_flNextCmdTime;
	int m_nServerCount;
	unsigned __int64 m_ulGameServerSteamID;
	int m_nCurrentSequence;
	CClockDriftMgr m_ClockDriftMgr;
	int m_nDeltaTick;
	bool m_bPaused;
	float m_flPausedExpireTime;
	int m_nViewEntity;
	int m_nPlayerSlot;
	char m_szLevelFileName[128];
	byte pad1[132];
	char m_szLevelBaseName[128];
	byte pad2[132];
	int m_nMaxClients;
	byte pad3[32768]; // PackedEntity* m_pEntityBaselines[2][MAX_EDICTS]
	byte pad4[2068];
	void* m_StringTableContainer;
	bool m_bRestrictServerCommands;
	bool m_bRestrictClientCommands;
	byte pad5[106];
	bool insimulation;
	int oldtickcount;
	float m_tickRemainder;
	float m_frameTime;
	int lastoutgoingcommand;
	int chokedcommands;
	int last_command_ack;
	int command_ack;
	int m_nSoundSequence;
	bool ishltv;
	bool isreplay;

	void ForceFullUpdate()
	{
		m_nDeltaTick = -1;
	}
};

class CSteamInterfaces
{
public:
	ISteamFriends* Friends = nullptr;
	ISteamApps* Apps = nullptr;
	ISteamUserStats* UserStats = nullptr;
	ISteamClient* Client = nullptr;
	ISteamUser* User = nullptr;
	ISteamUtils* Utils = nullptr;
	ISteamNetworkingUtils* NetworkingUtils = nullptr;

	void Init();
};

namespace I
{
	inline CBaseClientDLL* BaseClientDLL = nullptr;
	inline CClientDLLSharedAppSystems* ClientDLLSharedAppSystems = nullptr;
	inline CClientState* ClientState = nullptr;
	inline CClientModeShared* ClientModeShared = nullptr;
	inline CEngineClient* EngineClient = nullptr;
	inline IVEngineEffects* EngineEffects = nullptr;
	inline CPanel* VGuiPanel = nullptr;
	inline CSurface* VGuiSurface = nullptr;
	inline CClientEntityList* ClientEntityList = nullptr;
	inline IEffects* EffectsClient = nullptr;
	inline CModelInfoClient* ModelInfoClient = nullptr;
	inline CEngineTrace* EngineTrace = nullptr;
	inline CPrediction* Prediction = nullptr;
	inline CGameMovement* GameMovement = nullptr;
	inline IPhysics* Physics = nullptr;
	inline IPhysicsCollision* PhysicsCollision = nullptr;
	inline CMoveHelper* MoveHelper = nullptr;
	inline ICvar* Cvar = nullptr;
	inline CGlobalVarsBase* GlobalVars = nullptr;
	inline CEngineVGui* EngineVGui = nullptr;
	inline void* DemoPlayer = nullptr;
	inline IVRenderView* RenderView = nullptr;
	inline IViewRender* ViewRender = nullptr;
	inline CDebugOverlay* DebugOverlay = nullptr;
	inline CGameEventManager* GameEventManager = nullptr;
	inline CModelRender* ModelRender = nullptr;
	inline CMaterialSystem* MaterialSystem = nullptr;
	inline IInput* Input = nullptr;
	inline IKeyValuesSystem* KeyValuesSystem = nullptr;
	inline IUniformRandomStream* UniformRandomStream = nullptr;
	inline void* StudioRender = nullptr;
	inline IInputSystem* InputSystem = nullptr;
	inline void* CHud = nullptr;
	inline CTFGameMovement* TFGameMovement = nullptr;
	inline IAchievementMgr* AchievementMgr = nullptr;
	inline IViewRenderBeams* ViewRenderBeams = nullptr;
	inline IEngineSound* EngineSound = nullptr;
	inline CTFGameRules* TFGameRules = nullptr;
	inline void* BackpackPanel = nullptr;
	inline CTFGCClientSystem* TFGCClientSystem = nullptr;
	inline CTFPartyClient* TFPartyClient = nullptr;
	inline CTFInventoryManager* TFInventoryManager = nullptr;
	inline CThirdPersonManager* ThirdPersonManager = nullptr;
	inline IDirect3DDevice9* DirectXDevice = nullptr;
	inline ClientModeTFNormal* ClientModeTF = nullptr;
	inline ILocalize* Localize = nullptr;
	inline CCommonHostState* HostState = nullptr;
	inline ICenterPrint* CenterPrint = nullptr;
	inline IOcclusionSystem* OcclusionSystem = nullptr;	//	this is actually COcclusionSystem.

	// TODO: These do not belong here
	inline int32_t* RandomSeed = nullptr;
	inline bool* AllowSecureServers = nullptr;
}

class CInterfaces
{
public:
	void Init();
};

inline CInterfaces g_Interfaces;
inline CSteamInterfaces g_SteamInterfaces;
