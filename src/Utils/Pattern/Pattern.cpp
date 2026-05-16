#include "Pattern.h"
#include "../Minidump/Minidump.h"
#include <format>

#define IN_RANGE(x,a,b) (x >= a && x <= b)
#define GET_BITS(x) (IN_RANGE((x & (~0x20)),'A','F') ? ((x & (~0x20)) - 'A' + 0xA) : (IN_RANGE(x,'0','9') ? x - '0' : 0))
#define GET_BYTES(x) (GET_BITS(x[0]) << 4 | GET_BITS(x[1]))

uintptr_t CPattern::FindPattern(uintptr_t dwAddress, uintptr_t dwSize, LPCSTR szPattern)
{
	auto szPat = szPattern;
	uintptr_t dwFirstMatch = 0x0;
	const uintptr_t dwEnd = dwAddress + dwSize - strlen(szPattern);

	for (auto pCur = dwAddress; pCur < dwEnd; pCur++)
	{
		if (!*szPat)
		{
			return dwFirstMatch;
		}

		const auto pCurByte = *reinterpret_cast<const BYTE*>(pCur);
		const auto pBytePatt = *reinterpret_cast<const BYTE*>(szPat);

		if (pBytePatt == '\?' || pCurByte == GET_BYTES(szPat))
		{
			if (!dwFirstMatch)
			{
				dwFirstMatch = pCur;
			}

			//Found
			if (!szPat[2])
			{
				return dwFirstMatch;
			}

			szPat += (pBytePatt == '\?\?' || pBytePatt != '\?') ? 3 : 2;
		}
		else
		{
			szPat = szPattern;
			dwFirstMatch = 0x0;
		}
	}

	//Failed to find, return NULL
	return 0x0;
}

HMODULE CPattern::GetModuleHandleSafe(LPCSTR szModuleName)
{
	for (int waits = 0; ; waits++)
	{
		if (const auto hModule = GetModuleHandleA(szModuleName))
		{
			if (waits >= 100)
				FW_TRACE(std::format("[PATTERN] module became available | module={} waited_ms={}", szModuleName, waits * 10).c_str());

			return hModule;
		}

		if (waits == 100 || (waits > 100 && waits % 500 == 0))
		{
			FW_TRACE(std::format("[WARN] Waiting for pattern module | module={} waited_ms={} reason=module-not-loaded-yet", szModuleName, waits * 10).c_str());
		}

		Sleep(10);
	}
}

uintptr_t CPattern::Find(LPCSTR szModuleName, LPCSTR szPattern, bool logFailure)
{
	const auto modHandle = reinterpret_cast<uintptr_t>(GetModuleHandleSafe(szModuleName));
	if (!modHandle)
	{
		if (logFailure)
			FW_TRACE(std::format("[ERROR] Pattern module not loaded | module={} pattern={}", szModuleName, szPattern).c_str());
		return 0x0;
	}

	const auto* ntHeaders = reinterpret_cast<IMAGE_NT_HEADERS*>(modHandle + reinterpret_cast<IMAGE_DOS_HEADER*>(modHandle)->e_lfanew);
	const auto* optHeader = &ntHeaders->OptionalHeader;

	const auto address = FindPattern(modHandle + optHeader->BaseOfCode, optHeader->SizeOfCode, szPattern);
	if (!address && logFailure)
	{
		FW_TRACE(std::format("[ERROR] Raw pattern not found | module={} pattern={}", szModuleName, szPattern).c_str());
	}
	else if (address && logFailure)
	{
		FW_TRACE(std::format("[PATTERN] module={} address={} pattern={}", szModuleName, Minidump::DescribeAddress(address, true), szPattern).c_str());
	}

	return address;
}
