#include "../Hooks.h"



MAKE_HOOK(CServerGameClients_ProcessUsercmds, nullptr, float, __fastcall,
	void* ecx, void* edx, void* player, bf_read* buf, int numcmds, int totalcmds, int dropped_packets, bool ignore, bool paused)
{
	return Hook.Original<FN>()(ecx, edx, player, buf, numcmds, totalcmds, dropped_packets, ignore, paused);
}

