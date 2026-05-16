#include "Aimbot.h"
#include "../Vars.h"

#include "AimbotHitscan/AimbotHitscan.h"
#include "AimbotProjectile/AimbotProjectile.h"
#include "AimbotMelee/AimbotMelee.h"
#include "../Misc/Misc.h"
#include "../../Utils/FunctionalDebug/FunctionalDebug.h"

namespace
{
	void UpdateAimbotDebugState(const char* stage, const char* reason)
	{
		G::AimbotDebugStage = stage ? stage : "<null>";
		G::AimbotDebugReason = reason ? reason : "ok";
	}

	void TraceAimbotState(const char* stage, CBaseEntity* pLocal, CBaseCombatWeapon* pWeapon, const char* reason = "ok")
	{
		UpdateAimbotDebugState(stage, reason);

		static int sLastTraceTick = -1000000;
		const int tickCount = I::GlobalVars ? I::GlobalVars->tickcount : 0;
		if (tickCount - sLastTraceTick < 66)
		{
			return;
		}

		sLastTraceTick = tickCount;
		FW_TRACE(std::format(
			"[AIMBOT] stage={} reason={} weapon_id={} item_def={} slot={} type={} can_attack={} can_attack2={} can_headshot={} local_alive={} frozen={} cmd={} target_idx={}",
			stage ? stage : "<null>",
			reason ? reason : "ok",
			pWeapon ? pWeapon->GetWeaponID() : -1,
			pWeapon ? pWeapon->GetItemDefIndex() : -1,
			pWeapon ? pWeapon->GetSlot() : -1,
			static_cast<int>(G::CurWeaponType),
			G::WeaponCanAttack,
			G::WeaponCanSecondaryAttack,
			G::WeaponCanHeadShot,
			pLocal ? pLocal->IsAlive() : false,
			G::Frozen,
			G::CurrentUserCmd ? G::CurrentUserCmd->command_number : 0,
			G::CurrentTargetIdx
		).c_str());
	}
}

bool CAimbot::ShouldRun(CBaseEntity* pLocal, CBaseCombatWeapon* pWeapon)
{
	UpdateAimbotDebugState("ShouldRun", "ready");

	// Don't run while freecam is active
	if (G::FreecamActive) { FDBG_SKIP("Feature.Aimbot.ShouldRun", "FreecamActive"); TraceAimbotState("ShouldRun", pLocal, pWeapon, "freecam"); return false; }

	// Don't run if aimbot is disabled
	if (!Vars::Aimbot::Global::Active.Value) { FDBG_SKIP("Feature.Aimbot.ShouldRun", "Aimbot global inactive"); TraceAimbotState("ShouldRun", pLocal, pWeapon, "inactive"); return false; }

	// Don't run in menus
	if (I::EngineVGui->IsGameUIVisible()) { FDBG_SKIP("Feature.Aimbot.ShouldRun", "game UI visible"); TraceAimbotState("ShouldRun", pLocal, pWeapon, "game-ui-visible"); return false; }

	// Don't run if we are frozen in place.
	if (G::Frozen) { FDBG_SKIP("Feature.Aimbot.ShouldRun", "G::Frozen"); TraceAimbotState("ShouldRun", pLocal, pWeapon, "frozen"); return false; }

	if (!pLocal->IsAlive()
		|| pLocal->IsTaunting()
		|| pLocal->IsBonked()
		|| pLocal->GetFeignDeathReady()
		|| pLocal->IsCloaked()
		|| pLocal->IsInBumperKart()
		|| pLocal->IsAGhost())
	{
		FDBG_SKIP("Feature.Aimbot.ShouldRun", "local not attack-ready");
		TraceAimbotState("ShouldRun", pLocal, pWeapon, "local-not-ready");
		return false;
	}

	//	0 damage weapons that we still want to aimbot with
	if (pWeapon->GetWeaponID() == TF_WEAPON_BUILDER) {
		return true;
	}

	//	weapon data check for null damage
	if (CTFWeaponInfo* sWeaponInfo = pWeapon->GetTFWeaponInfo())
	{
		WeaponData_t sWeaponData = sWeaponInfo->m_WeaponData[0];
		if (sWeaponData.m_nDamage < 1)
		{
			FDBG_SKIP("Feature.Aimbot.ShouldRun", "weapon damage below 1");
			TraceAimbotState("ShouldRun", pLocal, pWeapon, "weapon-damage-zero");
			return false;
		}
	}
	else
	{
		FDBG_SKIP("Feature.Aimbot.WeaponDataCheck", "GetTFWeaponInfo null; continuing without damage check");
	}

	return true;
}

void CAimbot::Run(CUserCmd* pCmd)
{
	FDBG_SCOPE("Feature.Aimbot.RunBody");

	//G::CurrentTargetIdx = 0;
	G::PredictedPos = Vec3();
	G::HitscanRunning = false;
	G::HitscanSilentActive = false;
	G::AimPos = Vec3();

	if (F::Misc.bFastAccel)
	{
		FDBG_SKIP("Feature.Aimbot.RunBody", "fast accel active");
		TraceAimbotState("Run", nullptr, nullptr, "fast-accel");
		return;
	}

	const auto pLocal = g_EntityCache.GetLocal();
	const auto pWeapon = g_EntityCache.GetWeapon();
	if (!pLocal || !pWeapon) { FDBG_SKIP("Feature.Aimbot.RunBody", "local or weapon null"); return; }

	if (!ShouldRun(pLocal, pWeapon)) { return; }

	UpdateAimbotDebugState("Run", "routing");

	if (G::CurWeaponType == EWeaponType::UNKNOWN)
	{
		TraceAimbotState("Run", pLocal, pWeapon, "weapon-type-unknown");
	}

	if (SandvichAimbot::bIsSandvich = SandvichAimbot::IsSandvich())
	{
		G::CurWeaponType = EWeaponType::HITSCAN;
		TraceAimbotState("Run", pLocal, pWeapon, "sandvich-override");
	}

	switch (G::CurWeaponType)
	{
		case EWeaponType::HITSCAN:
		{
			UpdateAimbotDebugState("WeaponRouter", "hitscan");
			FDBG_CALL("Feature.AimbotHitscan.Run", F::AimbotHitscan.Run(pLocal, pWeapon, pCmd));
			break;
		}

		case EWeaponType::PROJECTILE:
		{
			UpdateAimbotDebugState("WeaponRouter", "projectile");
			FDBG_CALL("Feature.AimbotProjectile.Run", F::AimbotProjectile.Run(pLocal, pWeapon, pCmd));
			break;
		}

		case EWeaponType::MELEE:
		{
			UpdateAimbotDebugState("WeaponRouter", "melee");
			FDBG_CALL("Feature.AimbotMelee.Run", F::AimbotMelee.Run(pLocal, pWeapon, pCmd));
			break;
		}

		default:
		{
			FDBG_SKIP("Feature.Aimbot.WeaponRouter", "unknown weapon type");
			TraceAimbotState("WeaponRouter", pLocal, pWeapon, "unknown-weapon-type");
			break;
		}
	}
}
