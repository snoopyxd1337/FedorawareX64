#include "../Hooks.h"

// Credits: mfed
MAKE_HOOK(CTFMatchSummary_OnTick, S::CTFMatchSummary_OnTick(), int, __fastcall, void* ecx)
{
    return Hook.Original<FN>()(ecx);
}
