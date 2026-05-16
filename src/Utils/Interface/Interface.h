#pragma once

#include <Windows.h>

#include "../Minidump/Minidump.h"

#include <format>

class CInterface
{
private:
	typedef void*(*InstantiateInterface)();

	struct Interface_t
	{
		InstantiateInterface Interface;
		PCHAR szInterfaceName;
		Interface_t *NextInterface;
	};

public:
	template<typename T>
	__inline T Get(LPCSTR szModule, PCCH szObject)
	{
		if (const auto hModule = GetModuleHandleA(szModule))
		{
			if (const auto fnFactory = reinterpret_cast<void*(__cdecl*)(const char* pName, int* pReturnCode)>(GetProcAddress(hModule, "CreateInterface")))
			{
				int returnCode = 0;
				const auto result = fnFactory(szObject, &returnCode);
				if (!result)
				{
					FW_TRACE(std::format("[ERROR] Interface not found | module={} object={} returnCode={} reason=CreateInterface-returned-null", szModule, szObject, returnCode).c_str());
				}
				else
				{
					FW_TRACE(std::format("[INTERFACE] module={} object={} address={}", szModule, szObject, Minidump::DescribeAddress(reinterpret_cast<uintptr_t>(result))).c_str());
				}

				return reinterpret_cast<T>(result);
			}

			FW_TRACE(std::format("[ERROR] Interface factory missing | module={} object={} reason=CreateInterface-export-not-found", szModule, szObject).c_str());
			return nullptr;
		}

		FW_TRACE(std::format("[ERROR] Interface module missing | module={} object={} reason=module-not-loaded", szModule, szObject).c_str());
		return nullptr;
	}
};

inline CInterface g_Interface;
