#include "../Hooks.h"

#include "../../Features/Visuals/Visuals.h"
#include "../../Features/NoSpread/NoSpread.h"
#include "../../Features/Misc/Misc.h"
#include "../../Features/Chams/DMEChams.h"
#include "../../Features/Glow/Glow.h"
#include "../../Features/Backtrack/Backtrack.h"
#include "../../Features/TickHandler/TickHandler.h"
#include "../../Utils/FunctionalDebug/FunctionalDebug.h"


MAKE_HOOK(ViewRender_LevelShutdown, Utils::GetVFuncPtr(I::ViewRender, 2), void, __fastcall,
		  void* ecx)
{
	FDBG_FLUSH("LevelShutdown");
	FDBG_SCOPE_SLOW("Hook.ViewRender_LevelShutdown", FunctionalDebug::HOOK_SLOW_US);

	FW_TRACE("[LEVEL] ViewRender_LevelShutdown begin");
	FDBG_CALL("LevelShutdown.Original", Hook.Original<FN>()(ecx));

	FDBG_CALL("EntityCache.Clear", g_EntityCache.Clear());
	FDBG_CALL("Feature.Backtrack.Restart", F::Backtrack.Restart());
	FDBG_CALL("Feature.Ticks.Reset", F::Ticks.Reset());
	FDBG_CALL("Feature.NoSpread.Reset", F::NoSpread.Reset());
	G::CurrentUserCmd = nullptr;
	G::LastUserCmd = nullptr;
	G::ShouldShift = false;
	G::Teleporting = false;
	G::Recharging = false;

	FW_TRACE("[LEVEL] material cleanup begin");
	FDBG_CALL("Feature.Glow.Unload", F::Glow.Unload());
	FDBG_CALL("Feature.DMEChams.DeleteMaterials", F::DMEChams.DeleteMaterials());
	FW_TRACE("[LEVEL] material cleanup end");
	FDBG_CALL("Feature.Visuals.ClearMaterialHandles", F::Visuals.ClearMaterialHandles());
	//F::Statistics.Submit();
	FW_TRACE("[LEVEL] ViewRender_LevelShutdown end");
}
