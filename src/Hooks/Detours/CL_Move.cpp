#include "../Hooks.h"
#include "../../Features/TickHandler/TickHandler.h"
#include "../../Features/Vars.h"
#include "../../Utils/FunctionalDebug/FunctionalDebug.h"

MAKE_HOOK(CL_Move, S::CL_Move(), void, __cdecl, float accumulated_extra_samples, bool bFinalTick)
{
	FDBG_FLUSH("CL_Move");
	FDBG_SCOPE_SLOW("Hook.CL_Move", FunctionalDebug::HOOK_SLOW_US);

	if (!Vars::Misc::CL_Move::Enabled.Value || !I::EngineClient || !I::EngineClient->IsInGame() || I::EngineClient->IsPlayingTimeDemo())
	{
		FDBG_SKIP("Feature.Ticks.CLMove", std::format("passthrough enabled={} engine={} in_game={} timedemo={}",
			Vars::Misc::CL_Move::Enabled.Value,
			I::EngineClient != nullptr,
			I::EngineClient ? I::EngineClient->IsInGame() : false,
			I::EngineClient ? I::EngineClient->IsPlayingTimeDemo() : false));

		static int nLoggedPassthrough = 0;
		if (nLoggedPassthrough++ < 5)
		{
			FW_TRACE(std::format("[CL_MOVE] passthrough enabled={} in_game={} final_tick={}",
				Vars::Misc::CL_Move::Enabled.Value,
				I::EngineClient ? I::EngineClient->IsInGame() : false,
				bFinalTick).c_str());
		}

		FDBG_CALL("CL_Move.Original.Passthrough", Hook.Original<FN>()(accumulated_extra_samples, bFinalTick));
		if (I::EngineClient)
		{
			FDBG_CALL("CL_Move.EngineClient.FireEvents", I::EngineClient->FireEvents());
		}
		return;
	}

	FDBG_CALL("Feature.Ticks.CLMove", F::Ticks.CLMove(accumulated_extra_samples, bFinalTick));
	FDBG_CALL("CL_Move.EngineClient.FireEvents", I::EngineClient->FireEvents());
}
