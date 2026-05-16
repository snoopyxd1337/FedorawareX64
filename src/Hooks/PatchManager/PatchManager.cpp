#include "PatchManager.h"

#include "../../SDK/Signatures/Signatures.h"
#include "../../Utils/Pattern/Pattern.h"

void CPatchManager::Init()
{
	static BytePatch skyBoxFix{ g_Pattern.Find(ENGINE_DLL, "0F 82 ? ? ? ? 4A 63 84 2F"), {0x90, 0x90, 0x90, 0x90, 0x90, 0x90} };

	// Apply patches
	for (const auto& patch : GetVecPatches())
	{
		patch->Patch();
	}
}

void CPatchManager::Restore()
{
	for (const auto& patch : GetVecPatches())
	{
		patch->Restore();
	}

	GetVecPatches().clear();
}
