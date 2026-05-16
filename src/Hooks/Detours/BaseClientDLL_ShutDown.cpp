#include "../Hooks.h"

#include "../../Features/Visuals/Visuals.h"
#include "../../Features/Resolver/Resolver.h"
#include "../../Features/Misc/Misc.h"
#include "../../Features/Backtrack/Backtrack.h"
#include "../../Features/NoSpread/NoSpread.h"
#include "../../Features/TickHandler/TickHandler.h"

MAKE_HOOK(BaseClientDLL_Shutdown, Utils::GetVFuncPtr(I::BaseClientDLL, 7), void, __fastcall,
		  void* ecx)
{
	FW_TRACE("[LEVEL] BaseClientDLL_Shutdown begin");
	Hook.Original<FN>()(ecx);
	g_EntityCache.Clear();
	F::Backtrack.Restart();
	F::Ticks.Reset();
	F::NoSpread.Reset();
	F::Visuals.rain.Cleanup();
	G::DormantPlayerESP.clear();
	//F::Resolver.ResolveData.clear();
	G::ChokeMap.clear();
	G::CurrentUserCmd = nullptr;
	G::LastUserCmd = nullptr;
	G::ShouldShift = false;
	G::Teleporting = false;
	G::Recharging = false;
	FW_TRACE("[LEVEL] BaseClientDLL_Shutdown end");
}
