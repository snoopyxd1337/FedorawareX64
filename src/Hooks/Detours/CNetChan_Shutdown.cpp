#include "../Hooks.h"

namespace
{
	uintptr_t GetNetChannelShutdown()
	{
		if (!I::EngineClient)
		{
			return 0;
		}

		return Utils::GetVFuncPtr(I::EngineClient->GetNetChannelInfo(), 34);
	}
}

MAKE_HOOK(CNetChan_Shutdown, GetNetChannelShutdown(), void, __fastcall,
	CNetChannel* netChannel, const char* reason)
{
	FW_TRACE((std::string("[NET] CNetChan_Shutdown reason=") + (reason ? reason : "<null>")).c_str());
	return Hook.Original<FN>()(netChannel, reason);
}
