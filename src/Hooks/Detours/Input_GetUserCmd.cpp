#include "../Hooks.h"

MAKE_HOOK(Input_GetUserCmd, Utils::GetVFuncPtr(I::Input, 8), CUserCmd*, __fastcall,
		  void* ecx, int sequence_number)
{
	if (!I::Input || !I::Input->m_pCommands)
	{
		return Hook.Original<FN>()(ecx, sequence_number);
	}

	return &I::Input->m_pCommands[sequence_number % MULTIPLAYER_BACKUP];
}
