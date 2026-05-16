#include "../Hooks.h"

MAKE_HOOK(CNetGraphPanel_DrawTextFields, nullptr, void, __fastcall,
	void* ecx, void* edx, int graphvalue, int x, int y, int w, void* graph, void* cmdinfo)
{
	return Hook.Original<FN>()(ecx, edx, graphvalue, x, y, w, graph, cmdinfo);
}

