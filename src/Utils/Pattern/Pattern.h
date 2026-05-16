#pragma once
#include <Windows.h>

class CPattern
{
	uintptr_t FindPattern(uintptr_t dwAddress, uintptr_t dwLength, LPCSTR szPattern);
	HMODULE GetModuleHandleSafe(LPCSTR szModuleName);

public:
	uintptr_t Find(LPCSTR szModuleName, LPCSTR szPattern, bool logFailure = true);
};

inline CPattern g_Pattern;
