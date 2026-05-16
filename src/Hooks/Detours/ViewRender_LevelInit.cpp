#include "../Hooks.h"

#include "../../Features/Visuals/Visuals.h"
#include "../../Features/Backtrack/Backtrack.h"
#include "../../Features/TickHandler/TickHandler.h"
#include "../../Features/AntiHack/CheaterDetection/CheaterDetection.h"
#include "../../Features/Misc/Misc.h"
#include "../../Features/Chams/DMEChams.h"
#include "../../Features/Glow/Glow.h"
#include "../../Utils/FunctionalDebug/FunctionalDebug.h"

MAKE_HOOK(ViewRender_LevelInit, Utils::GetVFuncPtr(I::ViewRender, 1), void, __fastcall,
		  void* ecx)
{
	FDBG_FLUSH("LevelInit");
	FDBG_SCOPE_SLOW("Hook.ViewRender_LevelInit", FunctionalDebug::HOOK_SLOW_US);

	FW_TRACE("[LEVEL] ViewRender_LevelInit begin");
	FDBG_CALL("Feature.Visuals.StoreMaterialHandles", F::Visuals.StoreMaterialHandles());
	FDBG_CALL("Feature.Visuals.OverrideWorldTextures", F::Visuals.OverrideWorldTextures());
	//F::Statistics.Clear();

	FDBG_CALL("Feature.DMEChams.CreateMaterials", F::DMEChams.CreateMaterials());
	FDBG_CALL("Feature.Glow.Init", F::Glow.Init());

	FDBG_CALL("Feature.Backtrack.Restart", F::Backtrack.Restart());
	FDBG_CALL("Feature.Ticks.Reset", F::Ticks.Reset());
	FDBG_CALL("Feature.BadActors.OnLoad", F::BadActors.OnLoad());

	G::NextSafeTick = 0;

	FDBG_CALL("LevelInit.Original", Hook.Original<FN>()(ecx));

	FDBG_CALL("Feature.Visuals.ModulateWorld", F::Visuals.ModulateWorld());
	FW_TRACE("[LEVEL] ViewRender_LevelInit end");
}
