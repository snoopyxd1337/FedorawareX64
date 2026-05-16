#pragma once
#include <MinHook/MinHook.h>
#include <stdexcept>

#include "../SDK/SDK.h"
#include "../Utils/Minidump/Minidump.h"

//Credits go entirely to spook953

class CHook
{
	std::string m_Name;
	void* m_OriginalFunction = nullptr;
	void* m_InitFunction = nullptr;
	bool m_TargetWasNull = false;

public:
	CHook(const std::string& name, void* pInitFunction);

	const std::string& GetName() const
	{
		return m_Name;
	}

	void CreateHook(void* pTarget, void* pDetour)
	{
		m_TargetWasNull = false;
		if (!pTarget)
		{
			m_TargetWasNull = true;
			FW_TRACE(("[ERROR] Hook target is null: " + m_Name + " reason=target-address-resolved-to-0").c_str());
			return;
		}

		if (!pDetour)
		{
			FW_TRACE(("[ERROR] Hook detour is null: " + m_Name + " reason=detour-address-resolved-to-0").c_str());
			return;
		}

		FW_TRACE(("[HOOK] create begin name=" + m_Name + " target=" + Minidump::DescribeAddress(reinterpret_cast<uintptr_t>(pTarget), true)).c_str());
		FW_TRACE(("[HOOK] detour name=" + m_Name + " detour=" + Minidump::DescribeAddress(reinterpret_cast<uintptr_t>(pDetour), true)).c_str());

		if (!Minidump::IsExecutableAddress(reinterpret_cast<uintptr_t>(pTarget)))
			FW_TRACE(("[WARN] Hook target is not executable: " + m_Name).c_str());

		if (!Minidump::IsExecutableAddress(reinterpret_cast<uintptr_t>(pDetour)))
			FW_TRACE(("[WARN] Hook detour is not executable: " + m_Name).c_str());

		const auto status = MH_CreateHook(pTarget, pDetour, &m_OriginalFunction);
		if (status != MH_OK)
		{
#if defined(FW_DEBUG_DIAGNOSTICS)
			OutputDebugStringA(("Failed to create hook: " + m_Name + "\n").c_str());
#endif
			FW_TRACE(("[ERROR] Failed to create hook: " + m_Name + " status=" + std::to_string(status) + " text=" + MH_StatusToString(status)).c_str());
			return;
		}

		FW_TRACE(("[HOOK] create ok name=" + m_Name + " original=" + Minidump::DescribeAddress(reinterpret_cast<uintptr_t>(m_OriginalFunction))).c_str());
	}

	void DisableHook()
	{
		if (m_OriginalFunction && MH_DisableHook(m_OriginalFunction) != MH_OK)
		{
#if defined(FW_DEBUG_DIAGNOSTICS)
			OutputDebugStringA(("Failed to disable hook: " + m_Name + "\n").c_str());
#endif
			FW_TRACE(("[ERROR] Failed to disable hook: " + m_Name).c_str());
		}
	}

	bool HasOriginal() const
	{
		return m_OriginalFunction != nullptr;
	}

	bool TargetWasNull() const
	{
		return m_TargetWasNull;
	}

	void Init()
	{
		reinterpret_cast<void(__cdecl*)()>(m_InitFunction)();
	}

	template <typename FN>
	FN Original()
	{
		return reinterpret_cast<FN>(m_OriginalFunction);
	}
};

#define MAKE_HOOK(name, address, type, callconvo, ...) namespace Hooks \
{\
	namespace name\
	{\
		void Initialize();\
		inline CHook Hook(#name, Initialize); \
		using FN = type(callconvo*)(__VA_ARGS__); \
		type callconvo Detour(__VA_ARGS__); \
	}\
} \
void Hooks::name::Initialize() { Hook.CreateHook(reinterpret_cast<void*>(address), Detour); } \
type callconvo Hooks::name::Detour(__VA_ARGS__)
