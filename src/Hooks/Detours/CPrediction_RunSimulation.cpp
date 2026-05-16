#include "../Hooks.h"
#include "../../Features/TickHandler/TickHandler.h"
#include "../../Utils/FunctionalDebug/FunctionalDebug.h"

#include <vector>

struct TickbaseFix_t
{
	CUserCmd* pCmd = nullptr;
	int iLastOutgoingCommand = 0;
	int iTickbaseShift = 0;
};

static std::vector<TickbaseFix_t> s_vTickbaseFixes = {};

MAKE_HOOK(CPrediction_RunSimulation, S::CPrediction_RunSimulation(), void, __fastcall,
		  void* rcx, int current_command, float curtime, CUserCmd* cmd, CBaseEntity* localPlayer)
{
	FDBG_SCOPE("Hook.CPrediction_RunSimulation");

	if (localPlayer && cmd && I::ClientState)
	{
		if (Vars::Debug::TickbaseLogging.Value && F::Ticks.bShifting)
		{
			FW_TRACE(std::format("[TICKDBG] runsim seen cmd={} shifted={} start={} goal={} lastout={} ack={}",
				cmd->command_number,
				F::Ticks.GetShiftedTicks(),
				F::Ticks.GetShiftStart(),
				F::Ticks.GetShiftedGoal(),
				I::ClientState->lastoutgoingcommand,
				I::ClientState->last_command_ack).c_str());
		}

		if (F::Ticks.bShifting && F::Ticks.GetShiftedTicks() + 1 == F::Ticks.GetShiftStart())
		{
			const int iTickbaseShift = F::Ticks.GetShiftStart() - F::Ticks.GetShiftedGoal();
			if (iTickbaseShift > 0 && iTickbaseShift <= 24)
			{
				s_vTickbaseFixes.push_back({ G::CurrentUserCmd ? G::CurrentUserCmd : cmd, I::ClientState->lastoutgoingcommand, iTickbaseShift });
				F::Ticks.bShifting = false;
				if (Vars::Debug::TickbaseLogging.Value)
				{
					FW_TRACE(std::format("[TICKDBG] runsim queue cmd={} lastout={} shift={} start={} goal={}",
						cmd->command_number,
						I::ClientState->lastoutgoingcommand,
						iTickbaseShift,
						F::Ticks.GetShiftStart(),
						F::Ticks.GetShiftedGoal()).c_str());
				}
			}
		}

		for (auto it = s_vTickbaseFixes.begin(); it != s_vTickbaseFixes.end();)
		{
			if (it->iLastOutgoingCommand < I::ClientState->last_command_ack)
			{
				it = s_vTickbaseFixes.erase(it);
				continue;
			}

			if (cmd == it->pCmd)
			{
				localPlayer->SetTickBase(localPlayer->GetTickBase() - it->iTickbaseShift);
				if (Vars::Debug::TickbaseLogging.Value)
				{
					FW_TRACE(std::format("[TICKDBG] runsim apply cmd={} shift={} tickbase={}",
						cmd->command_number,
						it->iTickbaseShift,
						localPlayer->GetTickBase()).c_str());
				}
				break;
			}

			++it;
		}
	}

	Hook.Original<FN>()(rcx, current_command, curtime, cmd, localPlayer);
}
