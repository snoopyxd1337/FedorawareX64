#include "CritHack.h"

#include "../Aimbot/AimbotGlobal/AimbotGlobal.h"
#include <algorithm>
#include <cmath>
#define MASK_SIGNED 0x7FFFFFFF
#define WEAPON_RANDOM_RANGE 10000
#define TF_DAMAGE_CRIT_CHANCE 0.02f
#define TF_DAMAGE_CRIT_CHANCE_RAPID 0.02f
#define TF_DAMAGE_CRIT_CHANCE_MELEE 0.15f
#define TF_DAMAGE_CRIT_DURATION_RAPID 2.0f
#define TF_DAMAGE_CRIT_MULTIPLIER 3.0f
#define SEED_ATTEMPTS 4096
#define BUCKET_ATTEMPTS 1000

// i hate crithack

/* Returns whether random crits are enabled on the server */
bool CCritHack::AreRandomCritsEnabled()
{
	if (static auto tf_weapon_criticals = g_ConVars.FindVar("tf_weapon_criticals"); tf_weapon_criticals)
	{
		return tf_weapon_criticals->GetBool();
	}
	return true;
}

/* Returns whether the crithack should run */
bool CCritHack::IsEnabled()
{
	if (!Vars::CritHack::Active.Value) { return false; }
	if (!AreRandomCritsEnabled()) { return false; }
	if (!I::EngineClient->IsInGame()) { return false; }

	return true;
}

bool CCritHack::IsAttacking(const CUserCmd* pCmd, CBaseCombatWeapon* pWeapon)
{
	if (G::CurItemDefIndex == Soldier_m_TheBeggarsBazooka)
	{
		static bool bLoading = false;

		if (pWeapon->GetClip1() > 0)
		{
			bLoading = true;
		}

		if (!(pCmd->buttons & IN_ATTACK) && bLoading)
		{
			bLoading = false;
			return true;
		}
	}

	else
	{
		if (pWeapon->GetWeaponID() == TF_WEAPON_PIPEBOMBLAUNCHER)
		{
			static bool bCharging = false;

			if (pWeapon->GetChargeBeginTime() > 0.0f)
			{
				bCharging = true;
			}

			if (!(pCmd->buttons & IN_ATTACK) && bCharging)
			{
				bCharging = false;
				return true;
			}
		}
		else if (pWeapon->GetWeaponID() == TF_WEAPON_CANNON)
		{
			static bool Charging = false;

			if (pWeapon->GetDetonateTime() > 0.0f)
			{
				Charging = true;
			}

			if (!(pCmd->buttons & IN_ATTACK) && Charging)
			{
				Charging = false;
				return true;
			}
		}	

		//pssst..
		//Dragon's Fury has a gauge (seen on the weapon model) maybe it would help for pSilent hmm..
		/*
		if (pWeapon->GetWeaponID() == 109) {
		}*/

		else
		{
			if ((pCmd->buttons & IN_ATTACK) && G::WeaponCanAttack)
			{
				return true;
			}
		}
	}

	return false;
}

bool CCritHack::NoRandomCrits(CBaseCombatWeapon* pWeapon)
{
	if (!pWeapon)
	{
		return true;
	}

	const float critChance = Utils::ATTRIB_HOOK_FLOAT(1.0f, "mult_crit_chance", pWeapon, 0, true);
	if (critChance <= 0.0f)
	{
		return true;
	}

	//list of weapons that cant random crit, but dont have the attribute for it
	switch (pWeapon->GetWeaponID())
	{
		//scout
		case TF_WEAPON_PDA:
		case TF_WEAPON_BUILDER:
		case TF_WEAPON_INVIS:
		case TF_WEAPON_JAR_MILK:
		//soldier
		case TF_WEAPON_BUFF_ITEM:
		//pyro
		case TF_WEAPON_JAR_GAS:
		case TF_WEAPON_FLAME_BALL:
		case TF_WEAPON_ROCKETPACK:
		//demo
		case TF_WEAPON_PARACHUTE: //also for soldier
		//heavy
		case TF_WEAPON_LUNCHBOX:
		//engineer
		case TF_WEAPON_PDA_ENGINEER_BUILD:
		case TF_WEAPON_PDA_ENGINEER_DESTROY:
		case TF_WEAPON_LASER_POINTER:
		//medic
		case TF_WEAPON_MEDIGUN:
		//sniper
		case TF_WEAPON_SNIPERRIFLE:
		case TF_WEAPON_SNIPERRIFLE_CLASSIC:
		case TF_WEAPON_SNIPERRIFLE_DECAP:
		case TF_WEAPON_COMPOUND_BOW:
		case TF_WEAPON_JAR:
		//spy
		case TF_WEAPON_KNIFE:
		case TF_WEAPON_PDA_SPY_BUILD:
		case TF_WEAPON_PDA_SPY:
		case TF_WEAPON_PASSTIME_GUN:
			return true;
		default:
			return false;
	}
}

bool CCritHack::ShouldCrit()
{
	CBaseEntity* pLocal = g_EntityCache.GetLocal();
	if (!pLocal) { return false; }	//	will never hit
	static KeyHelper critKey{ &Vars::CritHack::CritKey.Value };
	if (critKey.Down() || !Vars::CritHack::CritKey.Value) { return true; }
	if (G::CurWeaponType == EWeaponType::MELEE && Vars::CritHack::AlwaysMelee.Value) { return true; }

	//Check if auto melee crit is enabled and player is using melee
	if (Vars::CritHack::AutoMeleeCrit.Value && G::CurWeaponType == EWeaponType::MELEE)
	{
		//Base melee damage
		int MeleeDamage = pLocal->GetClassNum() == ETFClass::CLASS_SCOUT ? 35 : 65;

		//Could be missing wepaons or reskins
		switch (G::CurItemDefIndex)
		{
		case Scout_t_SunonaStick:
		{
			//The Sun on a Stick has a -25% melee damage stat
			MeleeDamage = 26;
			break;
		}
		case Scout_t_TheFanOWar:
		{
			//The Fan O'War has a -75% melee damage stat
			MeleeDamage = 9;
			break;
		}
		case Scout_t_TheWrapAssassin:
		{
			//The Wrap Assassin has a -65% melee damage stat
			MeleeDamage = 12;
			break;
		}
		case Soldier_t_TheDisciplinaryAction:
		case Engi_t_TheJag:
		{
			//The Disciplinary Action and The Jag have a -25% melee damage stat
			MeleeDamage = 49;
			break;
		}
		case Soldier_t_TheEqualizer:
		{
			MeleeDamage = std::ceil(107.25 - (0.37295 * pLocal->GetHealth()));
			//The Equalizer does more damage the lower the local player's health is
			break;
		}
		case Pyro_t_HotHand:
		{
			//The Hot Hand has a -20% melee damage stat (and attacks twice)
			MeleeDamage = 56;
			break;
		}
		case Pyro_t_SharpenedVolcanoFragment:
		case Medic_t_Amputator:
		{
			//The Sharpened Volcano Fragment and The Amputator have a -20% melee damage stat
			MeleeDamage = 52;
			break;
		}
		case Pyro_t_TheBackScratcher:
		{
			//The Back Scratcher has a +25% melee damage stat
			MeleeDamage = 81;
			break;
		}
		case Demoman_t_TheScotsmansSkullcutter:
		{
			//The Scotsmans Skullcutter has a +20% melee damage stat
			MeleeDamage = 78;
			break;
		}
		case Heavy_t_WarriorsSpirit:
		{
			//The Warriors Spirit has a +30% melee damage stat
			MeleeDamage = 85;
			break;
		}
		case Sniper_t_TheTribalmansShiv:
		{
			//The Tribalmans Shiv has a -50% melee damage stat
			MeleeDamage = 37;
			break;
		}
		case Sniper_t_TheShahanshah:
		{
			const int iHalfHealth = pLocal->GetMaxHealth() / 2;
			MeleeDamage = pLocal->GetHealth() > iHalfHealth ? 49 : 81;
			//The Shahanshah has a -25% melee damage stat when above half health and a +25% when below half health
			//81 if below half health
			//49 if above half health
			break;
		}
		default: break;
		}

		CBaseEntity* pTarget = I::ClientEntityList->GetClientEntity(G::CurrentTargetIdx);
		if (pTarget)
		{
			if (G::CurItemDefIndex == Heavy_t_TheHolidayPunch)
			{
				if ((MeleeDamage >= pTarget->GetHealth()) && pTarget->IsVulnerable()) {
					return false;
				}
				else if (pTarget->OnSolid() && !pTarget->IsTaunting() && !pTarget->GetViewingCYOAPDA()) {
					return true;
				}
				else {
					return false;
				}
			}

			if (MeleeDamage <= pTarget->GetHealth())
				return true;
		}
	}

	return false;
}

int CCritHack::LastGoodCritTick(const CUserCmd* pCmd)
{
	if (!pCmd)
	{
		return -1;
	}

	CritTicks.erase(std::remove_if(CritTicks.begin(), CritTicks.end(), [pCmd](const int tick)
	{
		return tick < pCmd->command_number;
	}), CritTicks.end());

	if (CritTicks.empty())
	{
		return -1;
	}

	return CritTicks.front();
}

void CCritHack::UpdateWeaponInfo(CBaseEntity* pLocal, CBaseCombatWeapon* pWeapon)
{
	if (!pLocal || !pWeapon)
	{
		return;
	}

	m_iWeaponEntIndex = pWeapon->GetIndex();
	m_bMelee = pWeapon->GetSlot() == SLOT_MELEE;

	if (m_bMelee)
	{
		m_flCritChance = TF_DAMAGE_CRIT_CHANCE_MELEE * pLocal->GetCritMult();
	}
	else if (pWeapon->IsRapidFire())
	{
		m_flCritChance = TF_DAMAGE_CRIT_CHANCE_RAPID * pLocal->GetCritMult();
		const float nonCritDuration = (TF_DAMAGE_CRIT_DURATION_RAPID / m_flCritChance) - TF_DAMAGE_CRIT_DURATION_RAPID;
		m_flCritChance = nonCritDuration > 0.0f ? 1.0f / nonCritDuration : 0.0f;
	}
	else
	{
		m_flCritChance = TF_DAMAGE_CRIT_CHANCE * pLocal->GetCritMult();
	}

	m_flMultCritChance = Utils::ATTRIB_HOOK_FLOAT(1.0f, "mult_crit_chance", pWeapon, 0, true);
	m_flCritChance *= m_flMultCritChance;
}

int CCritHack::CommandToSeed(const int commandNumber)
{
	return MD5_PseudoRandom(commandNumber) & MASK_SIGNED;
}

bool CCritHack::IsCritSeed(const int seed, CBaseCombatWeapon* pWeapon, const bool crit, const bool safe)
{
	if (!pWeapon || seed == pWeapon->m_iCurrentSeed())
	{
		return false;
	}

	const bool engineCrit = EngineWillCritForSeed(pWeapon, seed);
	return crit ? engineCrit : !engineCrit;
}

bool CCritHack::IsCritCommand(const int commandNumber, CBaseCombatWeapon* pWeapon, const bool crit, const bool safe)
{
	return IsCritSeed(CommandToSeed(commandNumber), pWeapon, crit, safe);
}

#if defined(FW_DEBUG_DIAGNOSTICS)
bool CCritHack::CaptureSeedDebug(const int commandNumber, CBaseCombatWeapon* pWeapon, const bool crit, const bool safe)
{
	m_iDebugChosenCommand = commandNumber;
	m_iDebugRawSeed = MD5_PseudoRandom(commandNumber) & MASK_SIGNED;
	m_iDebugMaskedSeed = CommandToSeed(commandNumber);
	m_iDebugCurrentSeed = pWeapon ? pWeapon->m_iCurrentSeed() : 0;
	m_iDebugWeaponIndex = pWeapon ? pWeapon->GetIndex() : 0;
	m_iDebugLocalIndex = I::EngineClient->GetLocalPlayer();
	m_bDebugWantedCrit = crit;
	m_bDebugSafe = safe;

	Utils::RandomSeed(m_iDebugMaskedSeed);
	m_iDebugRandom = Utils::RandomInt(0, WEAPON_RANDOM_RANGE - 1);

	m_iDebugSafeLower = m_bMelee ? 1500 : 100;
	m_iDebugSafeUpper = m_bMelee ? 6000 : 800;
	m_iDebugSafeLower = static_cast<int>(m_iDebugSafeLower * m_flMultCritChance);
	m_iDebugSafeUpper = static_cast<int>(m_iDebugSafeUpper * m_flMultCritChance);
	m_iDebugRange = static_cast<int>(m_flCritChance * WEAPON_RANDOM_RANGE);
	m_bDebugEngineRaw = EngineWillCritForSeed(pWeapon, m_iDebugRawSeed);
	m_bDebugEngineMasked = EngineWillCritForSeed(pWeapon, m_iDebugMaskedSeed);

	if (!pWeapon || m_iDebugMaskedSeed == m_iDebugCurrentSeed)
	{
		m_bDebugResult = false;
		return false;
	}

	if (safe && (crit ? m_iDebugSafeLower >= 0 : m_iDebugSafeUpper < WEAPON_RANDOM_RANGE))
	{
		m_bDebugResult = crit ? m_iDebugRandom < m_iDebugSafeLower : !(m_iDebugRandom < m_iDebugSafeUpper);
		return m_bDebugResult;
	}

	m_bDebugResult = crit ? m_iDebugRandom < m_iDebugRange : !(m_iDebugRandom < m_iDebugRange);
	return m_bDebugResult;
}
#endif

bool CCritHack::EngineWillCritForSeed(CBaseCombatWeapon* pWeapon, const int seed)
{
	if (!pWeapon || !I::RandomSeed)
	{
		return false;
	}

	const int seedBackup = *I::RandomSeed;
	const float oldCritTokenBucket = pWeapon->m_flCritTokenBucket();
	const int oldCritChecks = pWeapon->m_nCritChecks();
	const int oldCritSeedRequests = pWeapon->m_nCritSeedRequests();
	const int oldCurrentSeed = pWeapon->m_iCurrentSeed();
	const float oldLastRapidFireCritCheckTime = pWeapon->m_flLastRapidFireCritCheckTime();
	const float oldCritTime = pWeapon->m_flCritTime();
	const int oldWeaponMode = pWeapon->m_iWeaponMode();

	ProtectData = true;
	pWeapon->m_iWeaponMode() = 0;
	*I::RandomSeed = seed;
	const bool result = pWeapon->WillCrit();
	*I::RandomSeed = seedBackup;
	pWeapon->m_iWeaponMode() = oldWeaponMode;
	pWeapon->m_flCritTokenBucket() = oldCritTokenBucket;
	pWeapon->m_nCritChecks() = oldCritChecks;
	pWeapon->m_nCritSeedRequests() = oldCritSeedRequests;
	pWeapon->m_iCurrentSeed() = oldCurrentSeed;
	pWeapon->m_flLastRapidFireCritCheckTime() = oldLastRapidFireCritCheckTime;
	pWeapon->m_flCritTime() = oldCritTime;
	ProtectData = false;

	return result;
}

int CCritHack::GetCritCommand(CBaseCombatWeapon* pWeapon, const int commandNumber, const bool crit, const bool safe)
{
	for (int i = commandNumber; i < commandNumber + SEED_ATTEMPTS; i++)
	{
		if (IsCritCommand(i, pWeapon, crit, safe))
		{
			return i;
		}
	}

	return 0;
}

void CCritHack::UpdateIndicatorInfo(CBaseEntity* pLocal, CBaseCombatWeapon* pWeapon)
{
	UpdateWeaponInfo(pLocal, pWeapon);

	m_flDamage = 0.0f;
	m_flCost = 0.0f;
	m_iAvailableCrits = 0;
	m_iPotentialCrits = 0;
	m_iNextCrit = 0;

	if (!pLocal || !pWeapon)
	{
		return;
	}

	static auto tf_weapon_criticals_bucket_cap = g_ConVars.FindVar("tf_weapon_criticals_bucket_cap");
	const float bucketCap = tf_weapon_criticals_bucket_cap ? tf_weapon_criticals_bucket_cap->GetFloat() : 1000.0f;
	const float bucket = pWeapon->m_flCritTokenBucket();
	const bool rapidFire = pWeapon->IsRapidFire();
	const float fireRate = std::max(pWeapon->GetFireRate(), I::GlobalVars->interval_per_tick);
	const WeaponData_t weaponData = pWeapon->GetWeaponData();

	float damage = static_cast<float>(std::max(weaponData.m_nDamage, 0));
	int projectilesPerShot = std::max(weaponData.m_nBulletsPerShot, 1);
	if (!m_bMelee && projectilesPerShot > 0)
	{
		projectilesPerShot = static_cast<int>(Utils::ATTRIB_HOOK_FLOAT(static_cast<float>(projectilesPerShot), "mult_bullets_per_shot", pWeapon, 0, true));
	}
	else
	{
		projectilesPerShot = 1;
	}

	float critDamage = damage * std::max(projectilesPerShot, 1);
	const float baseDamage = critDamage;
	if (baseDamage <= 0.0f)
	{
		return;
	}

	if (rapidFire)
	{
		critDamage *= TF_DAMAGE_CRIT_DURATION_RAPID / fireRate;
		if (critDamage * TF_DAMAGE_CRIT_MULTIPLIER > bucketCap)
		{
			critDamage = bucketCap / TF_DAMAGE_CRIT_MULTIPLIER;
		}
	}

	const auto remapVal = [](const float val, const float a, const float b, const float c, const float d)
	{
		return c + (d - c) * (val - a) / (b - a);
	};

	const int critChecks = pWeapon->m_nCritChecks();
	const int critSeedRequests = pWeapon->m_nCritSeedRequests();
	const float mult = m_bMelee ? 0.5f : remapVal(static_cast<float>(critSeedRequests + 1) / (critChecks + 1), 0.1f, 1.0f, 1.0f, 3.0f);
	const float cost = critDamage * TF_DAMAGE_CRIT_MULTIPLIER;
	const float denom = TF_DAMAGE_CRIT_MULTIPLIER * critDamage / (m_bMelee ? 2.0f : 1.0f) - baseDamage;

	m_flDamage = baseDamage;
	m_flCost = cost * mult;
	if (denom <= 0.0f)
	{
		return;
	}

	m_iPotentialCrits = std::max(0, static_cast<int>((std::max(bucketCap, bucket) - baseDamage) / denom));

	int testShots = critChecks;
	int testCrits = critSeedRequests;
	float testBucket = bucket;
	for (int i = 0; i < BUCKET_ATTEMPTS; i++)
	{
		testShots++;
		testCrits++;

		const float testMult = m_bMelee ? 0.5f : remapVal(static_cast<float>(testCrits) / testShots, 0.1f, 1.0f, 1.0f, 3.0f);
		if (testBucket < bucketCap)
		{
			testBucket = std::min(testBucket + baseDamage, bucketCap);
		}
		testBucket -= cost * testMult;
		if (testBucket < 0.0f)
		{
			break;
		}

		m_iAvailableCrits++;
	}

	m_iAvailableCrits = std::min(m_iAvailableCrits, m_iPotentialCrits);

	if (m_iAvailableCrits == m_iPotentialCrits)
	{
		return;
	}

	testShots = critChecks;
	testCrits = critSeedRequests;
	testBucket = bucket;
	float tickBase = static_cast<float>(pLocal->GetTickBase()) * I::GlobalVars->interval_per_tick;
	float lastRapidFireCritCheckTime = pWeapon->m_flLastRapidFireCritCheckTime();
	for (int i = 0; i < BUCKET_ATTEMPTS; i++)
	{
		int crits = 0;
		int testShots2 = testShots;
		int testCrits2 = testCrits;
		float testBucket2 = testBucket;
		for (int j = 0; j < BUCKET_ATTEMPTS; j++)
		{
			testShots2++;
			testCrits2++;

			const float testMult = m_bMelee ? 0.5f : remapVal(static_cast<float>(testCrits2) / testShots2, 0.1f, 1.0f, 1.0f, 3.0f);
			if (testBucket2 < bucketCap)
			{
				testBucket2 = std::min(testBucket2 + baseDamage, bucketCap);
			}
			testBucket2 -= cost * testMult;
			if (testBucket2 < 0.0f)
			{
				break;
			}

			crits++;
		}

		if (m_iAvailableCrits < crits)
		{
			break;
		}

		if (!rapidFire)
		{
			testShots++;
		}
		else
		{
			tickBase += std::ceilf(fireRate / I::GlobalVars->interval_per_tick) * I::GlobalVars->interval_per_tick;
			if (tickBase >= lastRapidFireCritCheckTime + 1.0f || (!i && testBucket == bucketCap))
			{
				testShots++;
				lastRapidFireCritCheckTime = tickBase;
			}
		}

		if (testBucket < bucketCap)
		{
			testBucket = std::min(testBucket + baseDamage, bucketCap);
		}

		m_iNextCrit++;
	}
}

void CCritHack::UpdateInfo(CBaseEntity* pLocal, CBaseCombatWeapon* pWeapon)
{
	UpdateIndicatorInfo(pLocal, pWeapon);

	m_bCritBanned = false;
	m_flDamageTilFlip = 0.0f;
	m_iResourceDamage = 0;
	m_iDesyncDamage = 0;

	if (!pLocal || !pWeapon)
	{
		return;
	}

	if (!m_bMelee)
	{
		const float normalizedCritDamage = static_cast<float>(m_iCritDamage) / TF_DAMAGE_CRIT_MULTIPLIER;
		const float allowedCritChance = m_flCritChance + 0.1f;

		if (m_iRangedDamage > 0 && m_iCritDamage > 0 && allowedCritChance > 0.0f)
		{
			const float observedDenom = normalizedCritDamage + static_cast<float>(m_iRangedDamage - m_iCritDamage);
			if (observedDenom > 0.0f)
			{
				const float observedCritChance = normalizedCritDamage / observedDenom;
				m_bCritBanned = observedCritChance > allowedCritChance;
			}
		}

		if (m_bCritBanned)
		{
			m_flDamageTilFlip = normalizedCritDamage / allowedCritChance + normalizedCritDamage * 2.0f - static_cast<float>(m_iRangedDamage);
		}
		else if (m_iCritDamage > 0 && allowedCritChance < 1.0f)
		{
			const float denom = allowedCritChance - 1.0f;
			m_flDamageTilFlip = TF_DAMAGE_CRIT_MULTIPLIER * (normalizedCritDamage - allowedCritChance * (normalizedCritDamage + static_cast<float>(m_iRangedDamage - m_iCritDamage))) / denom;
		}

		if (!std::isfinite(m_flDamageTilFlip) || m_flDamageTilFlip < 0.0f)
		{
			m_flDamageTilFlip = 0.0f;
		}
	}

	if (const auto& pResource = g_EntityCache.GetPR())
	{
		m_iResourceDamage = pResource->GetDamage(I::EngineClient->GetLocalPlayer());
		m_iDesyncDamage = m_iRangedDamage + m_iMeleeDamage - m_iResourceDamage;
	}
}

void CCritHack::ScanForCrits(const CUserCmd* pCmd, int loops)
{
	static int previousWeapon = 0;

	const auto& pLocal = g_EntityCache.GetLocal();
	if (!pLocal) { return; }

	const auto& pWeapon = pLocal->GetActiveWeapon();
	if (!pWeapon) { return; }

	if (F::AimbotGlobal.IsAttacking() || IsAttacking(pCmd, pWeapon)/* || pCmd->buttons & IN_ATTACK*/)
	{
		return;
	}

	if (previousWeapon != pWeapon->GetIndex())
	{
		previousWeapon = pWeapon->GetIndex();
		CritTicks.clear();
	}

	if (CritTicks.size() >= 256)
	{
		return;
	}

	UpdateWeaponInfo(pLocal, pWeapon);

	for (int i = 0; i < loops && CritTicks.size() < 256; i++)
	{
		const int commandNumber = pCmd->command_number + i;
		if (IsCritCommand(commandNumber, pWeapon))
		{
			CritTicks.push_back(commandNumber);
		}
	}

	std::sort(CritTicks.begin(), CritTicks.end());
	CritTicks.erase(std::unique(CritTicks.begin(), CritTicks.end()), CritTicks.end());
}

void CCritHack::Run(CUserCmd* pCmd)
{
	if (!IsEnabled()) { return; }

	const auto& pLocal = g_EntityCache.GetLocal();
	if (!pLocal || !pLocal->IsAlive()) { return; }

	const auto& pWeapon = g_EntityCache.GetWeapon();
	if (!pWeapon || NoRandomCrits(pWeapon) || !pWeapon->CanFireCriticalShot(false)) { return; }

	UpdateInfo(pLocal, pWeapon);
	ScanForCrits(pCmd, 256); //	fill our vector slowly.

	const bool attacking = IsAttacking(pCmd, pWeapon);
	const bool shouldCrit = ShouldCrit();

#if defined(FW_DEBUG_DIAGNOSTICS)
	m_iDebugOriginalCommand = pCmd->command_number;
	m_iDebugChosenCommand = 0;
	m_bDebugAttacking = attacking;
	m_bDebugShouldCrit = shouldCrit;
	m_szDebugStage = "ready";
#endif

	if (pWeapon->IsRapidFire() && I::GlobalVars->curtime < pWeapon->m_flLastRapidFireCritCheckTime() + 1.0f)
	{
#if defined(FW_DEBUG_DIAGNOSTICS)
		m_szDebugStage = "rapid-wait";
#endif
		return;
	}

	if (m_bCritBanned && !m_bMelee)
	{
#if defined(FW_DEBUG_DIAGNOSTICS)
		m_szDebugStage = "crit-banned";
#endif
		return;
	}

	if (attacking) //	is it valid & should we even use it
	{
		if (shouldCrit)
		{
			int critCommand = LastGoodCritTick(pCmd);
			if (critCommand < 0)
			{
				critCommand = GetCritCommand(pWeapon, pCmd->command_number);
			}

			if (critCommand <= 0)
			{
#if defined(FW_DEBUG_DIAGNOSTICS)
				m_szDebugStage = "no-crit-cmd";
				CaptureSeedDebug(pCmd->command_number, pWeapon, true);
				FW_TRACE(std::format("[CRITHACK] force failed stage={} orig={} weapon={} local={} chance={:.4f} mult={:.2f} bucket={:.1f} checks={} requests={} currentSeed={}",
					m_szDebugStage, pCmd->command_number, pWeapon->GetIndex(), I::EngineClient->GetLocalPlayer(), m_flCritChance, m_flMultCritChance,
					pWeapon->m_flCritTokenBucket(), pWeapon->m_nCritChecks(), pWeapon->m_nCritSeedRequests(), pWeapon->m_iCurrentSeed()).c_str());
#endif
				return;
			}

#if defined(FW_DEBUG_DIAGNOSTICS)
			CaptureSeedDebug(critCommand, pWeapon, true);
			m_szDebugStage = m_bDebugResult ? "force" : "force-bad-seed";
			FW_TRACE(std::format("[CRITHACK] force stage={} orig={} chosen={} delta={} rawSeed={} maskedSeed={} currentSeed={} random={} range={} safe=[{},{}] chance={:.4f} mult={:.2f} bucket={:.1f} cost={:.1f} available={} potential={}",
				m_szDebugStage, pCmd->command_number, critCommand, critCommand - pCmd->command_number, m_iDebugRawSeed, m_iDebugMaskedSeed,
				m_iDebugCurrentSeed, m_iDebugRandom, m_iDebugRange, m_iDebugSafeLower, m_iDebugSafeUpper, m_flCritChance, m_flMultCritChance,
				pWeapon->m_flCritTokenBucket(), m_flCost, m_iAvailableCrits, m_iPotentialCrits).c_str());
			FW_TRACE(std::format("[CRITHACK] engine-check rawWillCrit={} maskedWillCrit={} weapon={} local={} slot={} weaponID={} itemDef={} canCrit={} canAttack={}",
				m_bDebugEngineRaw, m_bDebugEngineMasked, pWeapon->GetIndex(), I::EngineClient->GetLocalPlayer(), pWeapon->GetSlot(),
				pWeapon->GetWeaponID(), pWeapon->GetItemDefIndex(), pWeapon->CanFireCriticalShot(false), G::WeaponCanAttack).c_str());
#endif
			pCmd->command_number = critCommand; //	set our cmdnumber to our wish
			pCmd->random_seed = MD5_PseudoRandom(critCommand) & MASK_SIGNED;
		}
		else if (Vars::CritHack::AvoidRandom.Value) //	we don't want to crit
		{
			if (const int skipCommand = GetCritCommand(pWeapon, pCmd->command_number, false))
			{
#if defined(FW_DEBUG_DIAGNOSTICS)
				CaptureSeedDebug(skipCommand, pWeapon, false);
				m_szDebugStage = m_bDebugResult ? "skip" : "skip-bad-seed";
				FW_TRACE(std::format("[CRITHACK] skip stage={} orig={} chosen={} delta={} rawSeed={} maskedSeed={} currentSeed={} random={} range={} safe=[{},{}]",
					m_szDebugStage, pCmd->command_number, skipCommand, skipCommand - pCmd->command_number, m_iDebugRawSeed, m_iDebugMaskedSeed,
					m_iDebugCurrentSeed, m_iDebugRandom, m_iDebugRange, m_iDebugSafeLower, m_iDebugSafeUpper).c_str());
				FW_TRACE(std::format("[CRITHACK] engine-check rawWillCrit={} maskedWillCrit={} weapon={} local={} slot={} weaponID={} itemDef={} canCrit={} canAttack={}",
					m_bDebugEngineRaw, m_bDebugEngineMasked, pWeapon->GetIndex(), I::EngineClient->GetLocalPlayer(), pWeapon->GetSlot(),
					pWeapon->GetWeaponID(), pWeapon->GetItemDefIndex(), pWeapon->CanFireCriticalShot(false), G::WeaponCanAttack).c_str());
#endif
				pCmd->command_number = skipCommand;
				pCmd->random_seed = MD5_PseudoRandom(skipCommand) & MASK_SIGNED;
			}
			else
			{
#if defined(FW_DEBUG_DIAGNOSTICS)
				m_szDebugStage = "no-skip-cmd";
				CaptureSeedDebug(pCmd->command_number, pWeapon, false);
#endif
			}
		}
	}
	else
	{
#if defined(FW_DEBUG_DIAGNOSTICS)
		m_szDebugStage = "not-attacking";
		CaptureSeedDebug(pCmd->command_number, pWeapon, true);
#endif
	}
}

void CCritHack::Draw()
{
	if (!Vars::CritHack::Indicators.Value) { return; }
	if (!IsEnabled() || !G::CurrentUserCmd) { return; }

	const auto& pLocal = g_EntityCache.GetLocal();
	if (!pLocal || !pLocal->IsAlive()) { return; }

	const auto& pWeapon = pLocal->GetActiveWeapon();
	if (!pWeapon) { return; }

	UpdateInfo(pLocal, pWeapon);

	const int x = Vars::CritHack::IndicatorPos.Value.c;
	int currentY = Vars::CritHack::IndicatorPos.Value.y;

	const float bucket = pWeapon->m_flCritTokenBucket();
	const int seedRequests = pWeapon->m_nCritSeedRequests();

	int longestW = 40;
	const auto& FONT= g_Draw.GetFont(FONT_INDICATORS);
	auto drawLine = [&](const std::wstring& text, const Color_t& color)
	{
		currentY += 15;
		g_Draw.String(FONT, x, currentY, color, ALIGN_CENTERHORIZONTAL, text.c_str());

		int w = 0, h = 0;
		I::VGuiSurface->GetTextSize(FONT.dwFont, text.c_str(), w, h);
		if (w > longestW)
		{
			longestW = w;
		}
	};

#if defined(FW_DEBUG_DIAGNOSTICS)
	if (Vars::Debug::DebugInfo.Value)
	{
		drawLine(std::format(L"{:#x}", reinterpret_cast<uintptr_t>(&pWeapon->m_flCritTokenBucket())), { 255, 255, 255, 255 });
	}
#endif
	if (!AreRandomCritsEnabled())
	{
		drawLine(L"Random crits disabled", { 255, 95, 95, 255 });
		IndicatorW = longestW * 2;
		IndicatorH = currentY;
		return;
	}
	//Can this weapon do random crits?
	if (NoRandomCrits(pWeapon) == true)
	{
		drawLine(L"No Random Crits", { 255, 95, 95, 255 });
		IndicatorW = longestW * 2;
		IndicatorH = currentY;
		return;
	}
	// Are we currently forcing crits?
	if (ShouldCrit())
	{
		drawLine(L"Forcing crits...", { 70, 190, 50, 255 });
	}

	const float tickBase = static_cast<float>(pLocal->GetTickBase()) * I::GlobalVars->interval_per_tick;
	if (pLocal->IsCritBoosted())
	{
		drawLine(L"Crit Boosted", { 120, 210, 255, 255 });
	}
	else if (pWeapon->m_flCritTime() > tickBase)
	{
		drawLine(std::format(L"Streaming crits {:.1f}s", pWeapon->m_flCritTime() - tickBase), { 120, 210, 255, 255 });
	}
	else if (m_bCritBanned)
	{
		drawLine(std::format(L"Deal {:.0f} damage", std::ceil(m_flDamageTilFlip)), { 255, 95, 95, 255 });
	}
	else if (m_iPotentialCrits <= 0)
	{
		drawLine(L"Crit Banned", { 255, 0, 0, 255 });
	}
	else if (CritTicks.empty())
	{
		drawLine(L"No crit seed", { 255, 95, 95, 255 });
	}
	else if (m_iAvailableCrits > 0)
	{
		if (!pWeapon->IsRapidFire() || tickBase >= pWeapon->m_flLastRapidFireCritCheckTime() + 1.0f)
		{
			drawLine(L"Crit Ready", { 70, 190, 50, 255 });
		}
		else
		{
			drawLine(std::format(L"Wait {:.1f}s", pWeapon->m_flLastRapidFireCritCheckTime() + 1.0f - tickBase), { 255, 195, 90, 255 });
		}
	}
	else
	{
		drawLine(std::format(L"Crit in {}{} shot{}", m_iNextCrit, m_iNextCrit == BUCKET_ATTEMPTS ? L"+" : L"", m_iNextCrit == 1 ? L"" : L"s"), { 255, 95, 95, 255 });
	}

	if (m_iPotentialCrits > 0)
	{
		drawLine(std::format(L"{}{} / {} crits", m_iAvailableCrits, m_iAvailableCrits == BUCKET_ATTEMPTS ? L"+" : L"", m_iPotentialCrits), { 181, 181, 181, 255 });
		if (m_iNextCrit > 0 && m_iAvailableCrits > 0)
		{
			drawLine(std::format(L"Next in {}{} shot{}", m_iNextCrit, m_iNextCrit == BUCKET_ATTEMPTS ? L"+" : L"", m_iNextCrit == 1 ? L"" : L"s"), { 181, 181, 181, 255 });
		}
	}

	if (m_flDamageTilFlip > 0.0f && !m_bCritBanned)
	{
		drawLine(std::format(L"{:.0f} damage", std::floor(m_flDamageTilFlip)), { 70, 190, 50, 255 });
	}

#if defined(FW_DEBUG_DIAGNOSTICS)
	if (m_iDesyncDamage != 0)
	{
		drawLine(std::format(L"Desync: {}", m_iDesyncDamage), { 255, 195, 90, 255 });
	}
#endif

	static auto tf_weapon_criticals_bucket_cap = g_ConVars.FindVar("tf_weapon_criticals_bucket_cap");
	const float bucketCap = tf_weapon_criticals_bucket_cap ? tf_weapon_criticals_bucket_cap->GetFloat() : 1000.0f;
	const std::wstring bucketstr = L"Bucket: " + std::to_wstring(static_cast<int>(bucket)) + L"/" + std::to_wstring(static_cast<int>(bucketCap));
	// crit bucket (this sucks)
	drawLine(bucketstr, { 181, 181, 181, 255 });
#if defined(FW_DEBUG_DIAGNOSTICS)
	if (Vars::Debug::DebugInfo.Value)
	{
		const std::wstring seedText = L"m_nCritSeedRequests: " + std::to_wstring(seedRequests);
		const std::wstring FoundCrits = L"Found Crit Ticks: " + std::to_wstring(CritTicks.size());
		const std::wstring commandNumber = L"cmdNumber: " + std::to_wstring(G::CurrentUserCmd->command_number);
		drawLine(seedText, { 181, 181, 181, 255 });
		drawLine(FoundCrits, { 181, 181, 181, 255 });
		drawLine(commandNumber, { 181, 181, 181, 255 });
		drawLine(std::format(L"Damage: {:.0f}, Cost: {:.0f}", m_flDamage, m_flCost), { 181, 181, 181, 255 });
		drawLine(std::format(L"CritChance: {:.2f}", m_flCritChance), { 181, 181, 181, 255 });
		drawLine(std::format(L"R/C/M: {}/{}/{}", m_iRangedDamage, m_iCritDamage, m_iMeleeDamage), { 181, 181, 181, 255 });
		drawLine(std::format(L"Res/Desync: {}/{}", m_iResourceDamage, m_iDesyncDamage), { 181, 181, 181, 255 });
		drawLine(std::format(L"Banned/Flip: {}/{:.0f}", m_bCritBanned ? 1 : 0, m_flDamageTilFlip), { 181, 181, 181, 255 });
		const std::wstring stageText = std::wstring(m_szDebugStage, m_szDebugStage + strlen(m_szDebugStage));
		drawLine(L"Stage: " + stageText, { 255, 255, 225, 255 });
		drawLine(std::format(L"Atk/Crit: {}/{}", m_bDebugAttacking ? 1 : 0, m_bDebugShouldCrit ? 1 : 0), { 255, 255, 225, 255 });
		drawLine(std::format(L"Cmd: {} -> {} ({})", m_iDebugOriginalCommand, m_iDebugChosenCommand, m_iDebugChosenCommand - m_iDebugOriginalCommand), { 255, 255, 225, 255 });
		drawLine(std::format(L"Seed: {} -> {}", m_iDebugRawSeed, m_iDebugMaskedSeed), { 255, 255, 225, 255 });
		drawLine(std::format(L"Roll: {} / range {}", m_iDebugRandom, m_iDebugRange), { 255, 255, 225, 255 });
		drawLine(std::format(L"Safe: {}..{} result {}", m_iDebugSafeLower, m_iDebugSafeUpper, m_bDebugResult ? 1 : 0), { 255, 255, 225, 255 });
		drawLine(std::format(L"Engine: raw {} mask {}", m_bDebugEngineRaw ? 1 : 0, m_bDebugEngineMasked ? 1 : 0), { 255, 255, 225, 255 });
		drawLine(std::format(L"CurSeed: {}", m_iDebugCurrentSeed), { 255, 255, 225, 255 });
		drawLine(std::format(L"Ent: weapon {} local {}", m_iDebugWeaponIndex, m_iDebugLocalIndex), { 255, 255, 225, 255 });
	}
#endif
	IndicatorW = longestW * 2;
	IndicatorH = currentY;
}

void CCritHack::Reset()
{
	CritTicks.clear();
	m_iCritDamage = 0;
	m_iRangedDamage = 0;
	m_iMeleeDamage = 0;
	m_iResourceDamage = 0;
	m_iDesyncDamage = 0;
	m_bCritBanned = false;
	m_flDamageTilFlip = 0.0f;
}

void CCritHack::Event(CGameEvent* pEvent, FNV1A_t uNameHash)
{
	if (!pEvent || !I::EngineClient->IsInGame())
	{
		return;
	}

	const int localIndex = I::EngineClient->GetLocalPlayer();

	if (uNameHash == FNV1A::HashConst("player_hurt"))
	{
		if (I::EngineClient->GetPlayerForUserID(pEvent->GetInt("attacker")) != localIndex
			|| I::EngineClient->GetPlayerForUserID(pEvent->GetInt("userid")) == localIndex)
		{
			return;
		}

		const int damage = pEvent->GetInt("damageamount");
		if (damage <= 0 || damage > 5000)
		{
			return;
		}

		const auto& pLocal = g_EntityCache.GetLocal();
		if (!pLocal)
		{
			return;
		}

		const int weaponID = pEvent->GetInt("weaponid");
		CBaseCombatWeapon* pEventWeapon = nullptr;
		for (int i = 0; i < MAX_WEAPONS; i++)
		{
			const auto& pWeapon = pLocal->GetWeaponFromSlot(i);
			if (pWeapon && pWeapon->GetWeaponID() == weaponID)
			{
				pEventWeapon = pWeapon;
				break;
			}
		}

		const bool melee = pEventWeapon && pEventWeapon->GetSlot() == SLOT_MELEE;
		if (melee)
		{
			m_iMeleeDamage += damage;
		}
		else
		{
			m_iRangedDamage += damage;
			if ((pEvent->GetBool("crit") || pEvent->GetBool("minicrit")) && !pLocal->IsCritBoosted())
			{
				m_iCritDamage += damage;
			}
		}

#if defined(FW_DEBUG_DIAGNOSTICS)
		if (Vars::Debug::DebugInfo.Value)
		{
			FW_TRACE(std::format("[CRITHACK] hurt damage={} crit={} minicrit={} weaponID={} melee={} ranged={} critDamage={} meleeDamage={}",
				damage, pEvent->GetBool("crit"), pEvent->GetBool("minicrit"), weaponID, melee, m_iRangedDamage, m_iCritDamage, m_iMeleeDamage).c_str());
		}
#endif
		return;
	}

	if (uNameHash == FNV1A::HashConst("scorestats_accumulated_update")
		|| uNameHash == FNV1A::HashConst("mvm_reset_stats")
		|| uNameHash == FNV1A::HashConst("teamplay_round_start"))
	{
		Reset();
		return;
	}

	if (uNameHash == FNV1A::HashConst("player_spawn")
		|| uNameHash == FNV1A::HashConst("player_changeclass"))
	{
		if (I::EngineClient->GetPlayerForUserID(pEvent->GetInt("userid")) == localIndex)
		{
			Reset();
		}
	}
}
