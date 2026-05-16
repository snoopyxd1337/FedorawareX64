#pragma once
#include "../Feature.h"
#include "../../Utils/Hash/FNV1A.h"

class CCritHack
{
private:
	bool AreRandomCritsEnabled();
	bool IsEnabled();
	bool ShouldCrit();
	bool NoRandomCrits(CBaseCombatWeapon* pWeapon);
	//bool ShouldForceMelee(CBaseCombatWeapon* pWeapon);	//	compare distances between local & enemies, force crits if we are within swing range of enemy.
	bool IsAttacking(const CUserCmd* pCmd, CBaseCombatWeapon* pWeapon);
	void UpdateWeaponInfo(CBaseEntity* pLocal, CBaseCombatWeapon* pWeapon);
	int CommandToSeed(int commandNumber);
	bool IsCritSeed(int seed, CBaseCombatWeapon* pWeapon, bool crit = true, bool safe = true);
	bool IsCritCommand(int commandNumber, CBaseCombatWeapon* pWeapon, bool crit = true, bool safe = true);
	int GetCritCommand(CBaseCombatWeapon* pWeapon, int commandNumber, bool crit = true, bool safe = true);
#if defined(FW_DEBUG_DIAGNOSTICS)
	bool CaptureSeedDebug(int commandNumber, CBaseCombatWeapon* pWeapon, bool crit = true, bool safe = true);
#endif
	bool EngineWillCritForSeed(CBaseCombatWeapon* pWeapon, int seed);
	void ScanForCrits(const CUserCmd* pCmd, int loops = 256);
	int LastGoodCritTick(const CUserCmd* pCmd);
	void UpdateIndicatorInfo(CBaseEntity* pLocal, CBaseCombatWeapon* pWeapon);
	void UpdateInfo(CBaseEntity* pLocal, CBaseCombatWeapon* pWeapon);
	void Reset();
	//int DamageToNextCrit(CBaseCombatWeapon* pWeapon);	//	returns a positive value if we are crit banned

	std::vector<int> CritTicks{};

	int m_iWeaponEntIndex = 0;
	bool m_bMelee = false;
	float m_flCritChance = 0.0f;
	float m_flMultCritChance = 1.0f;
	float m_flDamage = 0.0f;
	float m_flCost = 0.0f;
	int m_iAvailableCrits = 0;
	int m_iPotentialCrits = 0;
	int m_iNextCrit = 0;
	int m_iCritDamage = 0;
	int m_iRangedDamage = 0;
	int m_iMeleeDamage = 0;
	int m_iResourceDamage = 0;
	int m_iDesyncDamage = 0;
	bool m_bCritBanned = false;
	float m_flDamageTilFlip = 0.0f;

#if defined(FW_DEBUG_DIAGNOSTICS)
	int m_iDebugOriginalCommand = 0;
	int m_iDebugChosenCommand = 0;
	int m_iDebugRawSeed = 0;
	int m_iDebugMaskedSeed = 0;
	int m_iDebugRandom = 0;
	int m_iDebugRange = 0;
	int m_iDebugSafeLower = 0;
	int m_iDebugSafeUpper = 0;
	int m_iDebugCurrentSeed = 0;
	int m_iDebugWeaponIndex = 0;
	int m_iDebugLocalIndex = 0;
	bool m_bDebugResult = false;
	bool m_bDebugEngineRaw = false;
	bool m_bDebugEngineMasked = false;
	bool m_bDebugWantedCrit = true;
	bool m_bDebugSafe = true;
	bool m_bDebugAttacking = false;
	bool m_bDebugShouldCrit = false;
	const char* m_szDebugStage = "idle";
#endif

	//	TODO: Create & Restore to & from this struct when scanning for crits.
	//	Stop messing around with AddToBucket etc, just change values when scanning if needed.
	struct stats_t
	{
		float flCritBucket;	// 0xA54
		int iNumAttacks;	// 0xA58
		int iNumCrits;		// 0xA5C
	};

public:
	void Run(CUserCmd* pCmd);
	void Draw();
	void Event(CGameEvent* pEvent, FNV1A_t uNameHash);

	int IndicatorW;
	int IndicatorH;
	bool ProtectData = false;
};

ADD_FEATURE(CCritHack, CritHack)
