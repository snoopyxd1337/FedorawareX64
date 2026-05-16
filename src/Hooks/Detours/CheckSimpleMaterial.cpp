#include "../Hooks.h"

MAKE_HOOK(CheckSimpleMaterial, S::CheckSimpleMaterial(), bool, __cdecl, IMaterial* pMaterial)
{
	return Hook.Original<FN>()(pMaterial);
}
