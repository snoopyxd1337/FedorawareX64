#include "../Hooks.h"

//Credits to mfed for creating this entire thing
int ColorToInt(const Color_t& col)
{
    return col.r << 16 | col.g << 8 | col.b;
}

MAKE_HOOK(C_TFPlayer_FireEvent, S::CTFPlayer_FireEvent(), void, __fastcall,
    CBaseEntity* ecx, const Vector& origin, const QAngle& angles, int event_, const char* options)
{
    return Hook.Original<FN>()(ecx, origin, angles, event_, options);
}
