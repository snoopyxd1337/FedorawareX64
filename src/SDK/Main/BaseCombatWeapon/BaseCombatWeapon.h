#pragma once

#include "../BaseEntity/BaseEntity.h"

#ifndef TICKS_TO_TIME
#define TICKS_TO_TIME( t )	( I::GlobalVars->interval_per_tick * ( t ) )
#endif

namespace S
{
	MAKE_SIGNATURE(GetLocalizedBaseItemName, CLIENT_DLL, "48 8B C4 48 89 58 ? 48 89 70 ? 48 89 78 ? 55 41 54 41 56 41 57 48 8D 68 ? 48 81 EC ? ? ? ? 48 8B FA 48 8B D9 48 8B 01", 0x0);
	MAKE_SIGNATURE(GLocalizationProvider, CLIENT_DLL, "48 8B 05 ? ? ? ? 48 85 C0 74 ? 48 8B 00 48 8B C8 48 8B 10 FF 52 ? 48 8D 05", 0x0);
	MAKE_SIGNATURE(C_EconItemView_GetStaticData, CLIENT_DLL, "40 53 48 83 EC ? 48 8B D9 0F B7 41 ? 50", 0x0);
	MAKE_SIGNATURE(C_BaseCombatWeapon_GetName, CLIENT_DLL, "40 53 48 83 EC ? 48 8B D9 0F B7 81 ? ? ? ? 50", 0x0);

	MAKE_SIGNATURE(CBaseCombatWeapon_CanFireCriticalShot, CLIENT_DLL, "48 89 5C 24 ? 57 48 83 EC ? 48 8B F9 E8 ? ? ? ? 48 8B D8 48 85 C0 74 ? 48 8B 00", 0x0);
	MAKE_SIGNATURE(CBaseCombatWeapon_GetWeaponSpread, CLIENT_DLL, "48 89 5C 24 ? 57 48 83 EC ? 48 8B F9 E8 ? ? ? ? 48 8B D8 48 85 C0 74 ? 48 8B 00", 0x0);

	MAKE_SIGNATURE(CTFWeaponBaseMelee_DoSwingTraceInternal, CLIENT_DLL, "48 8B C4 48 89 58 ? 48 89 70 ? 48 89 78 ? 55 41 54 41 56 41 57 48 8D 68 ? 48 81 EC ? ? ? ? 48 8B F9", 0x0);

	MAKE_SIGNATURE(CBaseCombatWeapon_GetSpreadAngles, CLIENT_DLL, "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 0F 29 74 24 ? 48 8B DA 48 8B F9 E8 ? ? ? ? 48 8B C8", 0x0);
	MAKE_SIGNATURE(CBaseCombatWeapon_GetProjectileFireSetup, CLIENT_DLL, "48 8B C4 48 89 58 ? 48 89 70 ? 48 89 78 ? 55 41 54 41 56 41 57 48 8D 68 ? 48 81 EC ? ? ? ? 4C 8B F1", 0x0);

	MAKE_SIGNATURE(CBaseCombatWeapon_CalcIsAttackCritical, CLIENT_DLL, "48 89 74 24 ? 57 48 83 EC ? 48 8B F9 E8 ? ? ? ? 48 8B C8 C7 44 24 ? ? ? ? ? 4C 8D 0D ? ? ? ? 33 D2 4C 8D 05 ? ? ? ? E8 ? ? ? ? 48 8B F0 48 85 C0 0F 84 ? ? ? ? 48 8B 10", 0x0);
	MAKE_SIGNATURE(CBaseCombatWeapon_CalcIsAttackCriticalHelper, CLIENT_DLL, "40 53 48 83 EC ? 48 8B D9 E8 ? ? ? ? 48 8B C8 C7 44 24 ? ? ? ? ? 4C 8D 0D ? ? ? ? 33 D2 4C 8D 05 ? ? ? ? E8 ? ? ? ? 84 C0 74", 0x0);
	MAKE_SIGNATURE(CBaseCombatWeapon_CalcIsAttackCriticalHelperMelee, CLIENT_DLL, "40 53 48 83 EC ? 48 8B D9 E8 ? ? ? ? 48 8B C8 C7 44 24 ? ? ? ? ? 4C 8D 0D ? ? ? ? 33 D2 4C 8D 05 ? ? ? ? E8 ? ? ? ? 84 C0 74", 0x0);
}

class CBaseCombatWeapon : public CBaseEntity
{
public: //Netvars
	M_DYNVARGET(Clip1, int, this, "DT_BaseCombatWeapon", "LocalWeaponData", "m_iClip1")
		M_DYNVARGET(Clip2, int, this, "DT_BaseCombatWeapon", "LocalWeaponData", "m_iClip2")
		M_DYNVARGET(nViewModelIndex, int, this, "DT_BaseCombatWeapon", "LocalWeaponData", "m_nViewModelIndex")
		M_DYNVARGET(iViewModelIndex, int, this, "DT_BaseCombatWeapon", "m_iViewModelIndex")
		M_DYNVARGET(ItemDefIndex, int, this, "DT_EconEntity", "m_AttributeManager", "m_Item", "m_iItemDefinitionIndex")
		M_DYNVARGET(ChargeBeginTime, float, this, "DT_WeaponPipebombLauncher", "PipebombLauncherLocalData", "m_flChargeBeginTime")
		M_DYNVARGET(ChargeDamage, float, this, "DT_TFSniperRifle", "SniperRifleLocalData", "m_flChargedDamage")
		M_DYNVARGET(LastFireTime, float, this, "DT_TFWeaponBase", "LocalActiveTFWeaponData", "m_flLastFireTime")
		M_DYNVARGET(NextSecondaryAttack, float, this, "DT_BaseCombatWeapon", "LocalActiveWeaponData", "m_flNextSecondaryAttack")
		M_DYNVARGET(NextPrimaryAttack, float, this, "DT_BaseCombatWeapon", "LocalActiveWeaponData", "m_flNextPrimaryAttack")
		M_DYNVARGET(ChargeResistType, int, this, "DT_WeaponMedigun", "m_nChargeResistType")
		M_DYNVARGET(ReloadMode, int, this, "DT_TFWeaponBase", "m_iReloadMode")
		M_DYNVARGET(DetonateTime, float, this, "DT_WeaponGrenadeLauncher", "m_flDetonateTime")
		M_DYNVARGET(WeaponState, int, this, "DT_WeaponMinigun", "m_iWeaponState")
		//M_DYNVARGET(ObservedCritChance, float, this, "DT_LocalTFWeaponData", "m_flObservedCritChance")
		//M_DYNVARGET(LastCritCheckTime, float, this, "DT_TFWeaponBase", "LocalActiveTFWeaponData", "m_flLastCritCheckTime")
		M_DYNVARGET(ObservedCritChance, float, this, "DT_TFWeaponBase", "LocalActiveTFWeaponData", "m_flObservedCritChance")

		NETVAR(m_flChargeLevel, float, "CWeaponMedigun", "m_flChargeLevel")
		NETVAR(m_hHealingTarget, EHANDLE, "CWeaponMedigun", "m_hHealingTarget")
		NETVAR(m_bHealing, bool, "CWeaponMedigun", "m_bHealing")
		__inline float& GetCritTokenBucket() {
			static int nOffset = GetNetVar("CTFWeaponBase", "m_iReloadMode") - 244;
			return *reinterpret_cast<float*>(uintptr_t(this) + nOffset);
		}
		__inline float& m_flCritTokenBucket()
		{
			return GetCritTokenBucket();
		}

		__inline float GetUberCharge()
		{
			static auto dwOff = g_NetVars.get_offset("DT_WeaponMedigun", "m_flChargeLevel");
			return *reinterpret_cast<float*>(uintptr_t(this) + dwOff);
		}

		NETVAR(m_iPrimaryAmmoType, int, "CBaseCombatWeapon", "m_iPrimaryAmmoType");
	NETVAR(m_flNextPrimaryAttack, float, "CBaseCombatWeapon", "m_flNextPrimaryAttack")


		// pretty srue these are all wrong but i have no idea how to do the thing to find out what they are
	// you add 1c idiot
	/*
			*(float*)((uintptr_t)pWeapon + 0xA54) = crit_bucket;
	*(unsigned int*)((uintptr_t)pWeapon + 0xB58) = weapon_seed;
	*(unsigned int*)((uintptr_t)pWeapon + 0xB4c) = unknown1;
	*(unsigned int*)((uintptr_t)pWeapon + 0xB50) = unknown2;
	*(bool*)((uintptr_t)pWeapon + 0xB33) = unknown3;
	*(float*)((uintptr_t)pWeapon + 0xB5c) = unknown4;
	*(int*)((uintptr_t)pWeapon + 0xA58) = crit_attempts;
	*(int*)((uintptr_t)pWeapon + 0xA5c) = crit_count;
	*(float*)((uintptr_t)pWeapon + 0xC18) = observed_crit_chance;
	*(bool*)((uintptr_t)pWeapon + 0xB34) = unknown7;
	*//*
	M_OFFSETGET(WeaponSeed, int, 0xB58)
	M_OFFSETGET(Unknown1, int, 0xB4C)
	M_OFFSETGET(Unknown2, int, 0xB50)
	M_OFFSETGET(Unknown3, bool, 0xB33)
	M_OFFSETGET(Unknown4, float, 0xB5C)
	M_OFFSETGET(CritAttempts, int, 0xA58)
	M_OFFSETGET(CritCount, int, 0xA5C)
	M_OFFSETGET(ObservedCritChance, int, 0xC18)
	M_OFFSETGET(Unknown7, bool, 0xB34)
	M_OFFSETGET(WeaponMode, bool, 0xB20)
	M_OFFSETGET(WeaponDataa, bool, 0xB2C)*/


public: //Virtuals
	M_VIRTUALGET(WeaponID, int, this, int(*)(void*), 383)
		M_VIRTUALGET(Slot, int, this, int(*)(void*), 331)
		M_VIRTUALGET(DamageType, int, this, int(*)(void*), 384)
		M_VIRTUALGET(FinishReload, void, this, void(*)(void*), 277)
		M_VIRTUALGET(BulletSpread, Vec3&, this, Vec3& (*)(void*), 288)

public: //Everything else, lol
	__inline float GetSmackTime()
	{
		static auto dwOffset = GetNetVar("CTFWeaponBase", "m_nInspectStage") + 28;
		return *reinterpret_cast<float*>(uintptr_t(this) + dwOffset);
	}

	int GetBulletAmount();

	bool IsSpreadWeapon();

	__inline float ObservedCritChance()
	{
		DYNVAR_RETURN(float, this, "DT_TFWeaponBase", "LocalActiveTFWeaponData", "m_flObservedCritChance");
	}

		inline int& m_iWeaponMode()
	{
		static int offset = GetNetVar("CTFWeaponBase", "m_iReloadMode") - 4;
		return *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(this) + offset);
	}

	inline int& m_nCritChecks()
	{
		static int offset = GetNetVar("CTFWeaponBase", "m_iReloadMode") - 240;
		return *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(this) + offset);
	}

	inline int& m_nCritSeedRequests()
	{
		static int offset = GetNetVar("CTFWeaponBase", "m_iReloadMode") - 236;
		return *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(this) + offset);
	}

	inline int& m_iCurrentSeed()
	{
		static int offset = GetNetVar("CTFWeaponBase", "m_flLastCritCheckTime") + 8;
		return *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(this) + offset);
	}

	inline float& m_flCritTime()
	{
		static int offset = GetNetVar("CTFWeaponBase", "m_flLastCritCheckTime") - 4;
		return *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(this) + offset);
	}

	inline float& m_flLastRapidFireCritCheckTime()
	{
		static int offset = GetNetVar("CTFWeaponBase", "m_flLastCritCheckTime") + 12;
		return *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(this) + offset);
	}

	inline bool GetLocalizedBaseItemName(wchar_t(&szItemName)[128])
	{
		static auto fnGetLocalizedBaseItemName = S::GetLocalizedBaseItemName.As<bool(*)(wchar_t(&)[128], const void*, const void*)>();
		static auto fnGLocalizationProvider = S::GLocalizationProvider.As<void* (*)()>();
		static auto fnGetStaticData = S::C_EconItemView_GetStaticData.As<void* (*)(void*)>();

		void* pItem = m_Item();
		const void* pItemStaticData = fnGetStaticData(pItem);

		return fnGetLocalizedBaseItemName(szItemName, fnGLocalizationProvider(), pItemStaticData);
	}

	NETVAR(m_Item, void*, "CEconEntity", "m_Item");

	inline const char* GetName()
	{
		//static auto C_BaseCombatWeapon_GetName = S::C_BaseCombatWeapon_GetName.As<const char* (*)(void*)>();
		return GetVFunc<const char* (*)(void*)>(this, 334)(this);
		//return C_BaseCombatWeapon_GetName(this);
	}

	//str8 outta cathook
	__inline bool AmbassadorCanHeadshot()
	{
		if (GetItemDefIndex() == Spy_m_TheAmbassador || GetItemDefIndex() == Spy_m_FestiveAmbassador)
		{
			if ((I::GlobalVars->curtime - GetLastFireTime()) <= 1.0)
			{
				return false;
			}
		}
		return true;
	}

	__inline CAttributeList* GetAttributeList()
	{
		static auto dwOff = g_NetVars.get_offset("DT_EconEntity", "m_AttributeManager", "m_AttributeList");
		return reinterpret_cast<CAttributeList*>(uintptr_t(this) + dwOff);
	}

	__inline void SetItemDefIndex(const int nIndex)
	{
		static auto dwOff = g_NetVars.get_offset("DT_EconEntity", "m_AttributeManager", "m_Item", "m_iItemDefinitionIndex");
		*reinterpret_cast<int*>(uintptr_t(this) + dwOff) = nIndex;
	}

	__inline CBaseEntity* GetHealingTarget()
	{
		return reinterpret_cast<CBaseEntity*>(m_hHealingTarget().Get());
	}

	__inline EHANDLE GetHealingTargetHandle()
	{
		return m_hHealingTarget();
	}

	__inline WeaponData_t GetWeaponData()
	{
		if (const auto pWeaponInfo = GetTFWeaponInfo())
			return pWeaponInfo->m_WeaponData[0];

		return {};
	}

	__inline bool CanFireCriticalShot()
	{
		static auto fnCanFireCriticalShot = S::CBaseCombatWeapon_CanFireCriticalShot.As<bool(*)(CBaseCombatWeapon*)>();
		return fnCanFireCriticalShot ? fnCanFireCriticalShot(this) : false;
	}

	__inline CTFWeaponInfo* GetTFWeaponInfo()
	{
		static const int nOffset = GetNetVar("CTFWeaponBase", "m_flEffectBarRegenTime");
		if (!nOffset)
			return nullptr;

		return *reinterpret_cast<CTFWeaponInfo**>(reinterpret_cast<uintptr_t>(this) + nOffset + 16);
	}

	__inline float GetSwingRange(CBaseEntity* pLocal)
	{
		return static_cast<float>(GetVFunc<int(*)(CBaseEntity*)>(this, 455)(pLocal));
	}

	__inline float GetWeaponSpread()
	{
		static auto fnGetWeaponSpread = S::CBaseCombatWeapon_GetWeaponSpread.As<float(*)(decltype(this))>();
		return fnGetWeaponSpread(this);
	}

	__inline bool DoSwingTrace(CGameTrace& Trace)
	{
		return GetVFunc<int(*)(CGameTrace&)>(this, 454)(Trace);
	}

	__inline bool DoSwingTraceInternal(CGameTrace& Trace)
	{
		static auto fnDoSwingTraceInternal = S::CTFWeaponBaseMelee_DoSwingTraceInternal.As<bool(*)(decltype(this), CGameTrace&, bool, void*)>();
		if (fnDoSwingTraceInternal)
		{
			return fnDoSwingTraceInternal(this, Trace, false, nullptr);
		}

		static bool bLoggedMissing = false;
		if (!bLoggedMissing)
		{
			FW_TRACE("[WARN] CTFWeaponBaseMelee_DoSwingTraceInternal missing; falling back to vfunc DoSwingTrace");
			bLoggedMissing = true;
		}

		return DoSwingTrace(Trace);
	}

	__inline int LookupAttachment(const char* pAttachmentName)
	{
		const auto pRend = Renderable();
		return GetVFunc<int(*)(void*, const char*)>(pRend, 35)(pRend, pAttachmentName);
	}

	__inline bool GetAttachment(int number, Vec3& origin)
	{
		return GetVFunc<bool(*)(void*, int, Vec3&)>(this, 71)(this, number, origin);
	}


	__inline bool CanFireCriticalShot(const bool bHeadShot)
	{
		bool bResult = false;
		if (const auto& pOwner = I::ClientEntityList->GetClientEntityFromHandleSafe(GethOwner()))
		{
			const int nOldFov = pOwner->GetFov(); pOwner->SetFov(70);
			const auto fnCanFireCriticalShot = GetVFunc<bool(*)(decltype(this), bool, CBaseEntity*)>(this, 428);
			if (fnCanFireCriticalShot)
				bResult = fnCanFireCriticalShot(this, bHeadShot, nullptr);
			pOwner->SetFov(nOldFov);
		} return bResult;
	}

	__inline bool CanFireRandomCriticalShot(const float flCritChance)
	{
		return GetVFunc<bool(*)(decltype(this), float)>(this, 424)(this, flCritChance);
	}

	__inline bool CanWeaponHeadShot()
	{
		return ((GetDamageType() & DMG_USE_HITLOCATIONS) && CanFireCriticalShot(true)); //credits to bertti
	}

	__inline bool CanShoot(CBaseEntity* pLocal)
	{
		if (!pLocal->IsAlive() || pLocal->IsTaunting() || pLocal->IsBonked() || pLocal->IsAGhost() || pLocal->IsInBumperKart() || pLocal->m_fFlags() & FL_FROZEN)
			return false;

		if (pLocal->GetClassNum() == CLASS_SPY)
		{
			{ //DR
				static float flTimer = 0.0f;

				if (pLocal->GetFeignDeathReady())
				{
					flTimer = 0.0f;
					return false;
				}
				else
				{
					if (!flTimer)
						flTimer = I::GlobalVars->curtime;

					if (flTimer > I::GlobalVars->curtime)
						flTimer = 0.0f;

					if ((I::GlobalVars->curtime - flTimer) < 0.4f)
						return false;
				}
			}

			{ //Invis
				static float flTimer = 0.0f;

				if (pLocal->IsCloaked())
				{
					flTimer = 0.0f;
					return false;
				}
				else
				{
					if (!flTimer)
						flTimer = I::GlobalVars->curtime;

					if (flTimer > I::GlobalVars->curtime)
						flTimer = 0.0f;

					if ((I::GlobalVars->curtime - flTimer) < 2.0f)
						return false;
				}
			}
		}

		float flCurTime = static_cast<float>(pLocal->GetTickBase()) * I::GlobalVars->interval_per_tick;

		return GetNextPrimaryAttack() <= flCurTime && pLocal->GetNextAttack() <= flCurTime;
	}

	__inline bool CanSecondaryAttack(CBaseEntity* pLocal)
	{
		if (!pLocal->IsAlive() || pLocal->IsTaunting() || pLocal->IsBonked() || pLocal->IsAGhost() || pLocal->IsInBumperKart())
			return false;

		float flCurTime = static_cast<float>(pLocal->GetTickBase()) * I::GlobalVars->interval_per_tick;

		return GetNextSecondaryAttack() <= flCurTime && pLocal->GetNextAttack() <= flCurTime;
	}

	__inline bool IsInReload()
	{
		static DWORD dwNextPrimaryAttack = g_NetVars.get_offset("DT_BaseCombatWeapon", "LocalActiveWeaponData", "m_flNextPrimaryAttack");
		bool m_bInReload = *reinterpret_cast<bool*>(uintptr_t(this) + (dwNextPrimaryAttack + 0xC));
		static int nReloadMode = g_NetVars.get_offset("DT_TFWeaponBase", "m_iReloadMode");
		int m_iReloadMode = *reinterpret_cast<int*>(uintptr_t(this) + nReloadMode);
		return (m_bInReload || m_iReloadMode != 0);
	}

	__inline void GetSpreadAngles(Vec3& vOut)
	{
		static auto fnGetSpreadAngles = S::CBaseCombatWeapon_GetSpreadAngles.As<void(*)(decltype(this), Vec3&)>();
		fnGetSpreadAngles(this, vOut);
	}

	__inline void GetProjectileFireSetup(CBaseEntity* pPlayer, Vec3 vOffset, Vec3* vSrc, Vec3* vForward, bool bHitTeam, float flEndDist)
	{
		static auto fnGetProjectileFireSetu = S::CBaseCombatWeapon_GetProjectileFireSetup.As<void(*)(CBaseEntity*, CBaseEntity*, Vec3, Vec3*, Vec3*, bool, float)>();
		fnGetProjectileFireSetu(this, pPlayer, vOffset, vSrc, vForward, bHitTeam, flEndDist);
	}

	__inline bool IsRapidFire()
	{
		const bool ret = GetWeaponData().m_bUseRapidFireCrits;
		return ret || this->GetClientClass()->m_ClassID == static_cast<int>(ETFClassID::CTFMinigun);
	}



	__inline bool WillCrit()
	{
		return CalcIsAttackCriticalHelper();
	}

	__inline bool CalcIsAttackCritical()
	{
		using FN = bool(*)(void*);
		static auto fnCalcIsAttackCritical = S::CBaseCombatWeapon_CalcIsAttackCritical.As<FN>();
		
		return fnCalcIsAttackCritical ? fnCalcIsAttackCritical(this) : false;
	}

	__inline bool CalcIsAttackCriticalHelper()
	{
		if (const auto fnCalcIsAttackCriticalHelper = GetVFunc<bool(*)(decltype(this))>(this, 399))
		{
			return fnCalcIsAttackCriticalHelper(this);
		}

		using FN = bool(*)(CBaseCombatWeapon*);
		static FN fnCalcIsAttackCriticalHelper = S::CBaseCombatWeapon_CalcIsAttackCriticalHelper.As<FN>();
		return fnCalcIsAttackCriticalHelper ? fnCalcIsAttackCriticalHelper(this) : false;
	}

	__inline bool CalcIsAttackCriticalHelperMelee()
	{
		return CalcIsAttackCriticalHelper();
	}

	__inline void UpdateAllViewmodelAddons() {
		using FN = void(*)(CBaseCombatWeapon*);
		static FN fnUpdateAllViewmodelAddons = S::CTFWeaponBase_UpdateAllViewmodelAddons.As<FN>();
		return fnUpdateAllViewmodelAddons(this);
	}

	__inline bool CalcIsAttackCriticalHelperNoCrits(CBaseEntity* pWeapon)
	{
		typedef bool (*fn_t)(CBaseEntity*);
		return GetVFunc<fn_t>(pWeapon, 463, 0)(pWeapon);
	}

	//__inline bool CanFireCriticalShot(CBaseEntity* pWeapon)		// this does not fucking work no matter what i do and i have no idea why :DDD
	//{
	//	typedef bool (*fn_t)(CBaseEntity*, bool, CBaseEntity*);
	//	return GetVFunc<fn_t>(this, 491)(pWeapon, false, nullptr);
	//}

	__inline Vec3 GetSpreadAngles()
	{
		Vec3 vOut; GetSpreadAngles(vOut); return vOut;
	}

	__inline int GetMinigunState()
	{
		static int nOffset = g_NetVars.get_offset("DT_TFMinigun", "m_iWeaponState");
		return *reinterpret_cast<int*>(uintptr_t(this) + nOffset);
	}

	__inline bool IsReadyToFire()
	{
		static float lastFire = 0, nextAttack = 0;

		if (lastFire != GetLastFireTime())
		{
			lastFire = GetLastFireTime();
			nextAttack = GetNextPrimaryAttack();
		}

		if (GetClip1() == 0)
			return false;
		return (nextAttack <= (TICKS_TO_TIME(I::ClientEntityList->GetClientEntity(I::EngineClient->GetLocalPlayer())->GetTickBase())));
	}

	__inline bool IsFlipped()
	{
		static auto cl_flipviewmodels = I::Cvar->FindVar("cl_flipviewmodels");
		return cl_flipviewmodels->GetBool();
	}

	__inline float GetFireRate()
	{
		typedef float(__thiscall* FN)(PVOID);
		return GetVFunc<FN>(this, 359)(this);
	}

	CHudTexture* GetWeaponIcon();
};

class CTFWeaponInvis : public CBaseCombatWeapon
{
public:
	__inline bool HasFeignDeath()
	{
		return static_cast<bool>(GetVFunc<bool(*)(CTFWeaponInvis*)>(this, 522));
	}
};
