#include "../Hooks.h"
#include "../../Features/Backtrack/Backtrack.h"
#include "../../Features/Resolver/Resolver.h"
#include "../../Features/NoSpread/NoSpread.h"
#include "../../Features/AntiHack/CheaterDetection/CheaterDetection.h"

namespace S
{
	MAKE_SIGNATURE(C_TFWeaponBaseGun_FireBullet_Call, CLIENT_DLL, "0F 28 7C 24 ? 4C 8D 9C 24 ? ? ? ? 49 8B 5B ? 49 8B 6B ? 49 8B 73 ? 41 0F 28 73 ? 49 8B E3", 0x0);
}

MAKE_HOOK(FX_FireBullets, S::FX_FireBullets(), void, __cdecl,
		  void* pWpn, int iPlayer, const Vec3& vecOrigin, const Vec3& vecAngles, int iWeapon, int iMode, int iSeed, float flSpread, float flDamage, bool bCritical)
{
	if (iPlayer != I::EngineClient->GetLocalPlayer()){
		//Utils::ConLog("FX_FireBullets", tfm::format("%d {%.1f, %.1f, %.1f}", iPlayer, vecAngles.x, vecAngles.y, vecAngles.z).c_str(), {0, 222, 255, 255});
		F::Backtrack.ReportShot(iPlayer);
		F::Resolver.FXFireBullet(iPlayer, vecAngles);
		F::BadActors.ReportShot(iPlayer);
	}

	static const auto dwFireBulletCall = S::C_TFWeaponBaseGun_FireBullet_Call();
	if (iPlayer == I::EngineClient->GetLocalPlayer() && Vars::NoSpread::Hitscan.Value && reinterpret_cast<uintptr_t>(_ReturnAddress()) == dwFireBulletCall)
	{
		iSeed = F::NoSpread.GetSeed();
	}

	return Hook.Original<FN>()(pWpn, iPlayer, vecOrigin, vecAngles, iWeapon, iMode, iSeed, flSpread, flDamage, bCritical);
}
