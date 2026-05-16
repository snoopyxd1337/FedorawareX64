#pragma once
#include <Windows.h>

#include <cstddef>
#include <cstdint>
#include <string>

namespace Minidump
{
	void OpenConsole();
	void Trace(const char* message);
	std::string DescribeAddress(uintptr_t address, bool expectExecutable = false);
	bool IsReadableAddress(uintptr_t address, size_t size = sizeof(uintptr_t));
	bool IsExecutableAddress(uintptr_t address);
	void TraceAddress(const char* label, uintptr_t address, bool expectExecutable = false);
	void Initialize(HMODULE module);
	void Shutdown();
	LONG WINAPI ExceptionFilter(PEXCEPTION_POINTERS exPtr);
}

#if defined(FW_DEBUG_DIAGNOSTICS)
#define FW_TRACE(message) Minidump::Trace(message)
#define FW_TRACE_ADDRESS(label, address, expectExecutable) Minidump::TraceAddress(label, address, expectExecutable)
#else
#define FW_TRACE(message) ((void)0)
#define FW_TRACE_ADDRESS(label, address, expectExecutable) ((void)0)
#endif
