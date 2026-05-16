#include "Auto.h"

#include "../Vars.h"

#include "AutoShoot/AutoShoot.h"
#include "AutoStab/AutoStab.h"
#include "AutoDetonate/AutoDetonate.h"
#include "AutoBlast/AutoBlast.h"
#include "AutoUber/AutoUber.h"
#include "../../Utils/FunctionalDebug/FunctionalDebug.h"

namespace
{
	void UpdateTriggerDebugState(const char* reason)
	{
		G::TriggerDebugReason = reason ? reason : "ok";
	}

	void TraceAutoState(const char* reason, CBaseEntity* pLocal = nullptr, CBaseCombatWeapon* pWeapon = nullptr)
	{
		UpdateTriggerDebugState(reason);

		static int sLastTraceTick = -1000000;
		const int tickCount = I::GlobalVars ? I::GlobalVars->tickcount : 0;
		if (tickCount - sLastTraceTick < 66)
		{
			return;
		}

		sLastTraceTick = tickCount;
		FW_TRACE(std::format(
			"[TRIGGER] reason={} weapon_id={} item_def={} type={} can_attack={} key_down={} local_alive={} should_shift={}",
			reason ? reason : "ok",
			pWeapon ? pWeapon->GetWeaponID() : -1,
			pWeapon ? pWeapon->GetItemDefIndex() : -1,
			static_cast<int>(G::CurWeaponType),
			G::WeaponCanAttack,
			F::AutoGlobal.IsKeyDown(),
			pLocal ? pLocal->IsAlive() : false,
			G::ShouldShift
		).c_str());
	}
}

bool CAuto::ShouldRun(CBaseEntity* pLocal)
{
	UpdateTriggerDebugState("ready");

	/*
	if (!Vars::Triggerbot::Global::Active.m_Var || !F::AutoGlobal.IsKeyDown()) // this is bad because i say so
		return false;
	*/

	// if triggerbot is active and we havent set a key its clear we want to trigger all the time, forcing keybinds is madness (especially when it's not done @ AimbotGlobal.cpp)
	if (!Vars::Triggerbot::Global::Active.Value || (!F::AutoGlobal.IsKeyDown() && Vars::Triggerbot::Global::TriggerKey.
		Value))
	{
		FDBG_SKIP("Feature.Auto.ShouldRun", "triggerbot inactive or key not down");
		TraceAutoState("inactive-or-key", pLocal, g_EntityCache.GetWeapon());
		return false;
	}

	if (I::EngineVGui->IsGameUIVisible())
	{
		FDBG_SKIP("Feature.Auto.ShouldRun", "game UI visible");
		TraceAutoState("game-ui-visible", pLocal, g_EntityCache.GetWeapon());
		return false;
	}

	if (G::ShouldShift) { FDBG_SKIP("Feature.Auto.ShouldRun", "G::ShouldShift"); TraceAutoState("should-shift", pLocal, g_EntityCache.GetWeapon()); return false; }

	return true;
}

void CAuto::Run(CUserCmd* pCmd)
{
	FDBG_SCOPE("Feature.Auto.RunBody");

	if (Vars::Triggerbot::Stab::Disguise.Value && F::AutoStab.m_bShouldDisguise)
	{
		FDBG_CALL("Feature.Auto.LastDisguise", I::EngineClient->ClientCmd_Unrestricted("lastdisguise"));
	}

	G::AutoBackstabRunning = false;
	F::AutoStab.m_bShouldDisguise = false;

	const auto pLocal = g_EntityCache.GetLocal();
	const auto pWeapon = g_EntityCache.GetWeapon();

	if (pLocal && pWeapon)
	{
		if (ShouldRun(pLocal))
		{
			FDBG_CALL("Feature.AutoShoot.Run", F::AutoShoot.Run(pLocal, pWeapon, pCmd));
			FDBG_CALL("Feature.AutoStab.Run", F::AutoStab.Run(pLocal, pWeapon, pCmd));
			FDBG_CALL("Feature.AutoDetonate.Run", F::AutoDetonate.Run(pLocal, pWeapon, pCmd));
			FDBG_CALL("Feature.AutoAirblast.Run", F::AutoAirblast.Run(pLocal, pWeapon, pCmd));
			FDBG_CALL("Feature.AutoUber.Run", F::AutoUber.Run(pLocal, pWeapon, pCmd));
		}
	}
	else
	{
		FDBG_SKIP("Feature.Auto.RunBody", "local or weapon null");
	}
}
