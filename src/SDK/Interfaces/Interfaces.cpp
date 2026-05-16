#include "Interfaces.h"
#include "../../Utils/Minidump/Minidump.h"

#include <cstdint>
#include <format>

#if defined(FW_DEBUG_DIAGNOSTICS)
#define FW_DEBUG_OUTPUT_A(message) OutputDebugStringA(message)
#else
#define FW_DEBUG_OUTPUT_A(message) ((void)0)
#endif

#define VALIDATE(x) if (!(x)) { FW_DEBUG_OUTPUT_A("CInterfaces::Init() -> nullptr: " #x "\n"); FW_TRACE("[ERROR] CInterfaces::Init nullptr: " #x); } else { FW_TRACE(std::format("[INTERFACE] {} address={}", #x, Minidump::DescribeAddress(reinterpret_cast<uintptr_t>(x))).c_str()); }
#define VALIDATE_STEAM(x) if (!(x)) { FW_DEBUG_OUTPUT_A("CSteamInterfaces::Init() -> nullptr: " #x "\n"); FW_TRACE("[ERROR] CSteamInterfaces::Init nullptr: " #x); }

namespace
{
	uintptr_t ResolveRipRelative(CSignature& signature, uintptr_t offset = 3)
	{
		const auto instruction = signature();
		if (!instruction)
		{
			FW_TRACE(std::format("[ERROR] RIP-relative resolve failed | signature={} module={} reason=signature-null", signature.GetName(), signature.GetModule()).c_str());
			return 0;
		}

		if (!Minidump::IsReadableAddress(instruction + offset, sizeof(int32_t)))
		{
			FW_TRACE(std::format("[ERROR] RIP-relative displacement unreadable | signature={} instruction={} offset={}", signature.GetName(), Minidump::DescribeAddress(instruction, true), offset).c_str());
			return 0;
		}

		const auto displacement = *reinterpret_cast<int32_t*>(instruction + offset);
		const auto address = instruction + offset + sizeof(int32_t) + displacement;
		FW_TRACE(std::format("[RIP] signature={} instruction={} displacement={} target={}", signature.GetName(), Minidump::DescribeAddress(instruction, true), displacement, Minidump::DescribeAddress(address)).c_str());
		return address;
	}

	template <typename T>
	T GetRipRelative(CSignature& signature, int dereferenceCount, uintptr_t offset = 3)
	{
		auto address = ResolveRipRelative(signature, offset);
		for (int i = 0; address && i < dereferenceCount; i++)
		{
			if (!Minidump::IsReadableAddress(address, sizeof(uintptr_t)))
			{
				FW_TRACE(std::format("[ERROR] RIP-relative dereference unreadable | signature={} deref={} address={}", signature.GetName(), i + 1, Minidump::DescribeAddress(address)).c_str());
				return nullptr;
			}

			address = *reinterpret_cast<uintptr_t*>(address);
			FW_TRACE(std::format("[RIP] signature={} deref={} value={}", signature.GetName(), i + 1, Minidump::DescribeAddress(address)).c_str());
		}

		return reinterpret_cast<T>(address);
	}

	HMODULE GetSteamApiModule()
	{
		if (const auto steamApi64 = GetModuleHandleA("steam_api64.dll"))
		{
			return steamApi64;
		}

		return GetModuleHandleA("steam_api.dll");
	}
}

namespace S
{
	MAKE_SIGNATURE(GlobalVars_Interface, ENGINE_DLL, "48 8D 05 ? ? ? ? C3 CC CC CC CC CC CC CC CC 48 8B CA", 0x0);
	MAKE_SIGNATURE(ClientState_Interface, ENGINE_DLL, "48 8D 0D ? ? ? ? E8 ? ? ? ? F3 0F 5E 05", 0x0);
	MAKE_SIGNATURE(ClientModeShared, CLIENT_DLL, "48 8B 0D ? ? ? ? 48 8B 10 48 8B 19 48 8B C8 FF 92", 0x0);
	MAKE_SIGNATURE(DemoPlayer_Interface, ENGINE_DLL, "48 8B 05 ? ? ? ? 48 85 C0 74 ? 48 8B 00 48 8B C8 FF 50 ? 84 C0", 0x0);
	MAKE_SIGNATURE(TFGCClientSystem_Interface, CLIENT_DLL, "48 8D 05 ? ? ? ? C3 CC CC CC CC CC CC CC CC 48 8B 05 ? ? ? ? C3 CC CC CC CC CC CC CC CC 48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24", 0x0);
	MAKE_SIGNATURE(TFInventoryManager_Interface, CLIENT_DLL, "48 8D 0D ? ? ? ? E8 ? ? ? ? 48 8B D8 48 85 C0 0F 84 ? ? ? ? 48 8B 00", 0x0);
	MAKE_SIGNATURE(Input_Interface, CLIENT_DLL, "48 8B 0D ? ? ? ? 48 8B 01 FF 90 ? ? ? ? 85 C0 0F 84 ? ? ? ? F3 0F 10 05", 0x0);
	MAKE_SIGNATURE(UniformRandomStream_Interface, CLIENT_DLL, "48 8B 0D ? ? ? ? F3 0F 59 CA 44 8D 42", 0x0);
	MAKE_SIGNATURE(ViewRenderBeams_Interface, CLIENT_DLL, "48 8B 0D ? ? ? ? 48 8B D3 48 8B 01 FF 50 ? 0F B7 93", 0x0);
	MAKE_SIGNATURE(TFGameRules_Interface, CLIENT_DLL, "48 8B 0D ? ? ? ? 4C 8B C3 48 8B D7 48 8B 01 FF 90 ? ? ? ? 84 C0", 0x0);
	MAKE_SIGNATURE(ThirdPersonManager_Interface, CLIENT_DLL, "48 8D 0D ? ? ? ? 0F 29 BC 24", 0x0);
	MAKE_SIGNATURE(ClientModeTFNormal_Interface, CLIENT_DLL, "48 8D 0D ? ? ? ? E8 ? ? ? ? 48 8B 05 ? ? ? ? 48 8B 00", 0x0);
	MAKE_SIGNATURE(HostState_Interface, ENGINE_DLL, "48 8B 0D ? ? ? ? 48 85 C9 74 ? 80 39", 0x0);
	MAKE_SIGNATURE(CTFGameMovement_Interface, CLIENT_DLL, "48 8D 0D ? ? ? ? E8 ? ? ? ? 48 8D 0D ? ? ? ? E8 ? ? ? ? 48 8D 05", 0x0);
	MAKE_SIGNATURE(MoveHelper_Interface, CLIENT_DLL, "48 8B 0D ? ? ? ? 48 8B 01 FF 50 ? 0F B7 D7", 0x0);
	MAKE_SIGNATURE(Get_SteamNetworkingUtils, CLIENT_DLL, "40 53 48 83 EC ? 48 8B D9 48 8D 15 ? ? ? ? 33 C9 FF 15 ? ? ? ? 33 C9", 0x0);

	MAKE_SIGNATURE(Get_TFPartyClient, CLIENT_DLL, "48 8B 05 ? ? ? ? C3 CC CC CC CC CC CC CC CC 48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 41 56", 0x0);
	MAKE_SIGNATURE(DirectXDevice, "shaderapidx9.dll", "48 8B 0D ? ? ? ? 48 8B 01 FF 50 ? 8B F8", 0x0);
	MAKE_SIGNATURE(ViewRender, CLIENT_DLL, "48 8B 0D ? ? ? ? 48 8B 01 FF 90 ? ? ? ? 48 8B 03 48 8D 54 24", 0x0);
}

void CInterfaces::Init()
{
	using namespace I;

	BaseClientDLL = g_Interface.Get<CBaseClientDLL*>(CLIENT_DLL, CLIENT_DLL_INTERFACE_VERSION);
	VALIDATE(BaseClientDLL);

	ClientDLLSharedAppSystems = g_Interface.Get<CClientDLLSharedAppSystems*>(CLIENT_DLL, CLIENT_DLL_SHARED_APPSYSTEMS);
	VALIDATE(ClientDLLSharedAppSystems);

	ClientEntityList = g_Interface.Get<CClientEntityList*>(CLIENT_DLL, VCLIENTENTITYLIST_INTERFACE_VERSION);
	VALIDATE(ClientEntityList);

	Prediction = g_Interface.Get<CPrediction*>(CLIENT_DLL, VCLIENT_PREDICTION_INTERFACE_VERSION);
	VALIDATE(Prediction);

	GameMovement = g_Interface.Get<CGameMovement*>(CLIENT_DLL, CLIENT_GAMEMOVEMENT_INTERFACE_VERSION);
	VALIDATE(GameMovement);

	Physics = g_Interface.Get<IPhysics*>(VPHYSICS_DLL, VPHYSICS_INTERFACE_VERSION);
	VALIDATE(Physics);

	PhysicsCollision = g_Interface.Get<IPhysicsCollision*>(VPHYSICS_DLL, VPHYSICS_COLLISION_INTERFACE_VERSION);
	VALIDATE(PhysicsCollision);

	CenterPrint = g_Interface.Get<ICenterPrint*>(CLIENT_DLL, VCENTERPRINT_INTERFACE_VERSION);
	VALIDATE(CenterPrint);

	ModelInfoClient = g_Interface.Get<CModelInfoClient*>(ENGINE_DLL, VMODELINFO_CLIENT_INTERFACE_VERSION);
	VALIDATE(ModelInfoClient);

	EngineClient = g_Interface.Get<CEngineClient*>(ENGINE_DLL, VENGINE_CLIENT_INTERFACE_VERSION_13);
	VALIDATE(EngineClient);

	EngineEffects = g_Interface.Get<IVEngineEffects*>(ENGINE_DLL, VENGINE_EFFECTS_INTERFACE_VERSION);
	VALIDATE(EngineEffects);

	EngineTrace = g_Interface.Get<CEngineTrace*>(ENGINE_DLL, VENGINE_TRACE_CLIENT_INTERFACE_VERSION);
	VALIDATE(EngineTrace);

	VGuiPanel = g_Interface.Get<CPanel*>(VGUI2_DLL, VGUI_PANEL_INTERFACE_VERSION);
	VALIDATE(VGuiPanel);

	VGuiSurface = g_Interface.Get<CSurface*>(MATSURFACE_DLL, VGUI_SURFACE_INTERFACE_VERSION);
	VALIDATE(VGuiSurface);

	Cvar = g_Interface.Get<ICvar*>(VSTDLIB_DLL, VENGINE_CVAR_INTERFACE_VERSION);
	VALIDATE(Cvar);

	GlobalVars = GetRipRelative<CGlobalVarsBase*>(S::GlobalVars_Interface, 0);
	VALIDATE(GlobalVars);

	ClientState = GetRipRelative<CClientState*>(S::ClientState_Interface, 0);
	VALIDATE(ClientState);

	ClientModeShared = GetRipRelative<CClientModeShared*>(S::ClientModeShared, 1);
	VALIDATE(ClientModeShared);

	EngineVGui = g_Interface.Get<CEngineVGui*>(ENGINE_DLL, VENGINE_VGUI_VERSION);
	VALIDATE(EngineVGui);

	DemoPlayer = nullptr;

	RenderView = g_Interface.Get<IVRenderView*>(ENGINE_DLL, VENGINE_RENDERVIEW_INTERFACE_VERSION);
	VALIDATE(RenderView);

	DebugOverlay = g_Interface.Get<CDebugOverlay*>(ENGINE_DLL, VENGINE_DEBUGOVERLAY_INTERFACE_VERSION);
	VALIDATE(DebugOverlay);

	GameEventManager = g_Interface.Get<CGameEventManager*>(ENGINE_DLL, GAMEEVENTSMANAGER_ENGINE_INTERFACE);
	VALIDATE(GameEventManager);

	ModelRender = g_Interface.Get<CModelRender*>(ENGINE_DLL, VENGINE_MODELRENDER_INTERFACE);
	VALIDATE(ModelRender);

	MaterialSystem = g_Interface.Get<CMaterialSystem*>(MATSYSTEM_DLL, VMATERIALSYSTEM_INTERFACE);
	VALIDATE(MaterialSystem);

	TFGCClientSystem = GetRipRelative<CTFGCClientSystem*>(S::TFGCClientSystem_Interface, 0);
	VALIDATE(TFGCClientSystem);

	if (const auto fn = S::Get_TFPartyClient.As<CTFPartyClient*(*)()>())
	{
		TFPartyClient = fn();
	}
	VALIDATE(TFPartyClient);

	TFInventoryManager = nullptr;

	ViewRender = GetRipRelative<IViewRender*>(S::ViewRender, 1);
	VALIDATE(ViewRender);

	Input = GetRipRelative<IInput*>(S::Input_Interface, 1);
	VALIDATE(Input);

	auto GetKeyValuesSystem = [&]() -> IKeyValuesSystem* {
		const auto module = GetModuleHandleA(VSTDLIB_DLL);
		if (!module)
		{
			FW_TRACE("[ERROR] KeyValuesSystem resolve failed | reason=vstdlib-module-not-loaded");
			return nullptr;
		}

		static auto fn = reinterpret_cast<IKeyValuesSystem * (__cdecl*)()>(GetProcAddress(module, "KeyValuesSystem"));
		if (!fn)
		{
			FW_TRACE("[ERROR] KeyValuesSystem resolve failed | reason=export-not-found");
			return nullptr;
		}

		return fn();
	};

	KeyValuesSystem = GetKeyValuesSystem();
	VALIDATE(KeyValuesSystem);

	UniformRandomStream = GetRipRelative<IUniformRandomStream*>(S::UniformRandomStream_Interface, 1);
	VALIDATE(UniformRandomStream);

	StudioRender = g_Interface.Get<void*>(STUDIORENDER_DLL, "VStudioRender025");
	VALIDATE(StudioRender);

	InputSystem = g_Interface.Get<IInputSystem*>("inputsystem.dll", "InputSystemVersion001");
	VALIDATE(InputSystem);

	EffectsClient = g_Interface.Get<CEffectsClient*>(CLIENT_DLL, IEFFECTS_INTERFACE_VERSION);
	VALIDATE(EffectsClient);

	using getachievementmgr = IAchievementMgr* (*)();

	AchievementMgr = reinterpret_cast<IAchievementMgr*>(GetVFunc<getachievementmgr>(EngineClient, 115));
	VALIDATE(AchievementMgr);

	ViewRenderBeams = GetRipRelative<IViewRenderBeams*>(S::ViewRenderBeams_Interface, 1);
	VALIDATE(ViewRenderBeams);

	EngineSound = g_Interface.Get<IEngineSound*>(ENGINE_DLL, "IEngineSoundClient003");
	VALIDATE(EngineSound);

	TFGameRules = nullptr;

	ThirdPersonManager = GetRipRelative<CThirdPersonManager*>(S::ThirdPersonManager_Interface, 0);
	VALIDATE(ThirdPersonManager);

	DirectXDevice = GetRipRelative<IDirect3DDevice9*>(S::DirectXDevice, 1);
	VALIDATE(DirectXDevice);

	ClientModeTF = nullptr;

	Localize = g_Interface.Get<ILocalize*>(VGUI2_DLL, VGUI_LOCALIZE_INTERFACE_VERSION);
	VALIDATE(Localize);

	HostState = nullptr;

	TFGameMovement = static_cast<CTFGameMovement*>(GameMovement);
	VALIDATE(TFGameMovement);

	MoveHelper = GetRipRelative<CMoveHelper*>(S::MoveHelper_Interface, 1);
	VALIDATE(MoveHelper);

	RandomSeed = GetRipRelative<int32_t*>(S::RandomSeed, 0);
	VALIDATE(RandomSeed);

	AllowSecureServers = GetRipRelative<bool*>(S::AllowSecureServers, 0, 11);
	VALIDATE(AllowSecureServers);
}

void CSteamInterfaces::Init()
{
	Client = g_Interface.Get<ISteamClient*>("steamclient64.dll", STEAMCLIENT_INTERFACE_VERSION);
	if (!Client)
	{
		Client = g_Interface.Get<ISteamClient*>("steamclient.dll", STEAMCLIENT_INTERFACE_VERSION);
	}
	VALIDATE_STEAM(Client);
	if (!Client) { return; }

	if (const auto steamApi = GetSteamApiModule())
	{
		using GetHSteamUserFn = HSteamUser(*)();
		using GetHSteamPipeFn = HSteamPipe(*)();

		const auto getSteamUser = reinterpret_cast<GetHSteamUserFn>(GetProcAddress(steamApi, "SteamAPI_GetHSteamUser"));
		const auto getSteamPipe = reinterpret_cast<GetHSteamPipeFn>(GetProcAddress(steamApi, "SteamAPI_GetHSteamPipe"));

		const HSteamUser hSteamUser = getSteamUser ? getSteamUser() : 0;
		const HSteamPipe hSteamPipe = getSteamPipe ? getSteamPipe() : 0;

		if (hSteamUser && hSteamPipe)
		{
			Friends = Client->GetISteamFriends(hSteamUser, hSteamPipe, STEAMFRIENDS_INTERFACE_VERSION);
			Utils = Client->GetISteamUtils(hSteamPipe, STEAMUTILS_INTERFACE_VERSION);
			Apps = Client->GetISteamApps(hSteamUser, hSteamPipe, STEAMAPPS_INTERFACE_VERSION);
			UserStats = Client->GetISteamUserStats(hSteamUser, hSteamPipe, STEAMUSERSTATS_INTERFACE_VERSION);
			User = Client->GetISteamUser(hSteamUser, hSteamPipe, STEAMUSER_INTERFACE_VERSION);

			if (Friends && Utils && Apps && UserStats && User)
			{
				FW_TRACE("[STEAM] using existing SteamAPI user/pipe");
			}
			else
			{
				FW_TRACE("[WARN] SteamAPI user/pipe available, but one or more Steam interfaces were null; falling back to a private pipe");
			}
		}
		else
		{
			FW_TRACE("[WARN] SteamAPI current user/pipe unavailable; falling back to a private pipe");
		}
	}
	else
	{
		FW_TRACE("[WARN] steam_api module not found; falling back to a private pipe");
	}

	if (!(Friends && Utils && Apps && UserStats && User))
	{
		const HSteamPipe hsNewPipe = Client->CreateSteamPipe();
		VALIDATE_STEAM(hsNewPipe);
		if (!hsNewPipe) { return; }

		const HSteamUser hsNewUser = Client->ConnectToGlobalUser(hsNewPipe);
		VALIDATE_STEAM(hsNewUser);
		if (!hsNewUser) { return; }

		Friends = Client->GetISteamFriends(hsNewUser, hsNewPipe, STEAMFRIENDS_INTERFACE_VERSION);
		VALIDATE_STEAM(Friends);

		Utils = Client->GetISteamUtils(hsNewPipe, STEAMUTILS_INTERFACE_VERSION);
		VALIDATE_STEAM(Utils);

		Apps = Client->GetISteamApps(hsNewUser, hsNewPipe, STEAMAPPS_INTERFACE_VERSION);
		VALIDATE_STEAM(Apps);

		UserStats = Client->GetISteamUserStats(hsNewUser, hsNewPipe, STEAMUSERSTATS_INTERFACE_VERSION);
		VALIDATE_STEAM(UserStats);

		User = Client->GetISteamUser(hsNewUser, hsNewPipe, STEAMUSER_INTERFACE_VERSION);
		VALIDATE_STEAM(User);
		FW_TRACE("[STEAM] using private Steam pipe fallback");
	}

	// Credits to spook953 for teaching me how this works
	using FN = ISteamNetworkingUtils* (__fastcall*)(ISteamNetworkingUtils**);
	if (const auto fn = S::Get_SteamNetworkingUtils.As<FN>())
	{
		if (const auto networkingUtils = fn(&NetworkingUtils))
		{
			NetworkingUtils = networkingUtils;
		}
	}

	if (!NetworkingUtils)
	{
		if (const auto steamNetworkingSockets = GetModuleHandleA("steamnetworkingsockets.dll"))
		{
			if (const auto fn = reinterpret_cast<ISteamNetworkingUtils * (__cdecl*)()>(GetProcAddress(steamNetworkingSockets, "SteamNetworkingUtils_LibV4")))
			{
				NetworkingUtils = fn();
			}
		}
	}
	VALIDATE_STEAM(NetworkingUtils);
}
