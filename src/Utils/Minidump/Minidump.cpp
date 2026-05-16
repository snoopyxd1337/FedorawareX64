#include "Minidump.h"

#include <dbghelp.h>
#include <shlobj.h>
#include <array>
#include <cstdio>
#include <ctime>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#define STATUS_CPP_EH_EXCEPTION ((DWORD)0xE06D7363)
#ifndef DBG_PRINTEXCEPTION_C
#define DBG_PRINTEXCEPTION_C ((DWORD)0x40010006)
#endif
#ifndef DBG_PRINTEXCEPTION_WIDE_C
#define DBG_PRINTEXCEPTION_WIDE_C ((DWORD)0x4001000A)
#endif

#if defined(FW_DEBUG_DIAGNOSTICS)
#define FW_MINIDUMP_DIAGNOSTICS 1
#else
#define FW_MINIDUMP_DIAGNOSTICS 0
#endif

namespace
{
	PVOID g_ExceptionHandler = nullptr;
	HMODULE g_Module = nullptr;
	bool g_DiagnosticsWindowStarted = false;
	LONG g_FilterActive = 0;
	LONG g_ExceptionTraceCount = 0;
	std::array<void*, 32> g_LoggedAddresses = {};
	bool g_LoggedNullAddress = false;

	std::wstring GetModuleDirectory();
	std::wstring GetCrashOutputDirectory();
	void EnsureCrashOutputDirectory();

	std::wstring GetTracePath()
	{
		return GetCrashOutputDirectory() + L"\\Fedoraware_load_trace.txt";
	}

	std::wstring GetDiagnosticsScriptPath()
	{
		return GetCrashOutputDirectory() + L"\\Fedoraware_diagnostics.cmd";
	}

	void EnsureTraceFileExists()
	{
		EnsureCrashOutputDirectory();
		const HANDLE file = CreateFileW(GetTracePath().c_str(), FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (file != INVALID_HANDLE_VALUE)
			CloseHandle(file);
	}

	bool WriteDiagnosticsScript()
	{
		const auto scriptPath = GetDiagnosticsScriptPath();
		const HANDLE file = CreateFileW(scriptPath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (file == INVALID_HANDLE_VALUE)
			return false;

		const char script[] =
			"@echo off\r\n"
			"title Fedoraware diagnostics\r\n"
			"cd /d \"%~dp0\"\r\n"
			"echo Fedoraware diagnostics console active\r\n"
			"echo Watch for ERROR and WARN lines.\r\n"
			"echo Full trace lives in the local crashlogs folder next to the DLL.\r\n"
			"echo.\r\n"
			"powershell -NoProfile -ExecutionPolicy Bypass -Command \"$p = Join-Path $PWD.Path 'Fedoraware_load_trace.txt'; while (!(Test-Path -LiteralPath $p)) { Start-Sleep -Milliseconds 250 }; Get-Content -LiteralPath $p -Tail 200 -Wait | Where-Object { $_ -notmatch 'MainThread heartbeat' }\"\r\n";

		DWORD written = 0;
		const BOOL ok = WriteFile(file, script, static_cast<DWORD>(strlen(script)), &written, nullptr);
		CloseHandle(file);
		return ok == TRUE;
	}

	bool LaunchDiagnosticsWindow()
	{
		if (g_DiagnosticsWindowStarted)
			return true;

		EnsureTraceFileExists();
		if (!WriteDiagnosticsScript())
			return false;

		auto scriptPath = GetDiagnosticsScriptPath();
		std::wstring commandLine = L"cmd.exe /k \"\"" + scriptPath + L"\"\"";

		STARTUPINFOW startupInfo = {};
		startupInfo.cb = sizeof(startupInfo);
		startupInfo.dwFlags = STARTF_USESHOWWINDOW;
		startupInfo.wShowWindow = SW_SHOWNOACTIVATE;

		PROCESS_INFORMATION processInfo = {};
		const BOOL created = CreateProcessW(
			nullptr,
			commandLine.data(),
			nullptr,
			nullptr,
			FALSE,
			CREATE_NEW_CONSOLE,
			nullptr,
			nullptr,
			&startupInfo,
			&processInfo
		);

		if (!created)
			return false;

		CloseHandle(processInfo.hThread);
		CloseHandle(processInfo.hProcess);
		g_DiagnosticsWindowStarted = true;
		return true;
	}

	void WriteLine(const std::wstring& path, const char* message)
	{
		SYSTEMTIME time = {};
		GetLocalTime(&time);

		char prefix[128] = {};
		sprintf_s(
			prefix,
			"[%04u-%02u-%02u %02u:%02u:%02u.%03u] [pid:%lu tid:%lu] ",
			time.wYear,
			time.wMonth,
			time.wDay,
			time.wHour,
			time.wMinute,
			time.wSecond,
			time.wMilliseconds,
			GetCurrentProcessId(),
			GetCurrentThreadId()
		);

		std::string line = prefix;
		line += message ? message : "(null)";
		line += "\r\n";

		const HANDLE file = CreateFileW(path.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (file == INVALID_HANDLE_VALUE)
			return;

		DWORD written = 0;
		WriteFile(file, line.data(), static_cast<DWORD>(line.size()), &written, nullptr);
		CloseHandle(file);
	}

	std::wstring GetModuleDirectory()
	{
		wchar_t path[MAX_PATH] = {};
		if (g_Module && GetModuleFileNameW(g_Module, path, MAX_PATH))
		{
			std::wstring result(path);
			const auto pos = result.find_last_of(L"\\/");
			if (pos != std::wstring::npos)
				result.resize(pos);
			return result;
		}

		if (GetTempPathW(MAX_PATH, path))
			return path;

		return L".";
	}

	std::wstring GetCrashOutputDirectory()
	{
		return GetModuleDirectory() + L"\\crashlogs";
	}

	void EnsureCrashOutputDirectory()
	{
		const auto outputDir = GetCrashOutputDirectory();
		CreateDirectoryW(outputDir.c_str(), nullptr);
	}

	std::string Narrow(const std::wstring& value)
	{
		if (value.empty())
			return {};

		const int size = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, nullptr, 0, nullptr, nullptr);
		if (size <= 1)
			return {};

		std::string result(size - 1, '\0');
		WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, result.data(), size, nullptr, nullptr);
		return result;
	}

	std::string GetFileName(const std::string& path)
	{
		const auto index = path.find_last_of("\\/");
		return index == std::string::npos ? path : path.substr(index + 1);
	}

	std::string GetModuleName(HMODULE module)
	{
		char path[MAX_PATH] = {};
		if (module && GetModuleFileNameA(module, path, MAX_PATH))
			return GetFileName(path);
		return {};
	}

	bool TryGetModuleOffset(uintptr_t address, std::string& result)
	{
		HMODULE module = nullptr;
		if (!GetModuleHandleExA(
			GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
			reinterpret_cast<LPCSTR>(address),
			&module))
		{
			return false;
		}

		const auto base = reinterpret_cast<uintptr_t>(module);
		std::ostringstream stream;
		stream << GetModuleName(module) << "+0x" << std::hex << (address - base);
		result = stream.str();
		return true;
	}

	DWORD BaseProtection(DWORD protect)
	{
		return protect & 0xFF;
	}

	const char* ProtectName(DWORD protect)
	{
		switch (BaseProtection(protect))
		{
		case PAGE_EXECUTE: return "PAGE_EXECUTE";
		case PAGE_EXECUTE_READ: return "PAGE_EXECUTE_READ";
		case PAGE_EXECUTE_READWRITE: return "PAGE_EXECUTE_READWRITE";
		case PAGE_EXECUTE_WRITECOPY: return "PAGE_EXECUTE_WRITECOPY";
		case PAGE_NOACCESS: return "PAGE_NOACCESS";
		case PAGE_READONLY: return "PAGE_READONLY";
		case PAGE_READWRITE: return "PAGE_READWRITE";
		case PAGE_WRITECOPY: return "PAGE_WRITECOPY";
		default: return "UNKNOWN";
		}
	}

	const char* StateName(DWORD state)
	{
		switch (state)
		{
		case MEM_COMMIT: return "MEM_COMMIT";
		case MEM_FREE: return "MEM_FREE";
		case MEM_RESERVE: return "MEM_RESERVE";
		default: return "UNKNOWN";
		}
	}

	const char* TypeName(DWORD type)
	{
		switch (type)
		{
		case MEM_IMAGE: return "MEM_IMAGE";
		case MEM_MAPPED: return "MEM_MAPPED";
		case MEM_PRIVATE: return "MEM_PRIVATE";
		default: return "UNKNOWN";
		}
	}

	bool IsReadableProtect(DWORD protect)
	{
		if (protect & (PAGE_GUARD | PAGE_NOACCESS))
			return false;

		switch (BaseProtection(protect))
		{
		case PAGE_READONLY:
		case PAGE_READWRITE:
		case PAGE_WRITECOPY:
		case PAGE_EXECUTE_READ:
		case PAGE_EXECUTE_READWRITE:
		case PAGE_EXECUTE_WRITECOPY:
			return true;
		default:
			return false;
		}
	}

	bool IsExecutableProtect(DWORD protect)
	{
		if (protect & (PAGE_GUARD | PAGE_NOACCESS))
			return false;

		switch (BaseProtection(protect))
		{
		case PAGE_EXECUTE:
		case PAGE_EXECUTE_READ:
		case PAGE_EXECUTE_READWRITE:
		case PAGE_EXECUTE_WRITECOPY:
			return true;
		default:
			return false;
		}
	}

	bool QueryCommittedRange(uintptr_t address, size_t size, MEMORY_BASIC_INFORMATION& memoryInfo)
	{
		if (!address)
			return false;

		if (!VirtualQuery(reinterpret_cast<LPCVOID>(address), &memoryInfo, sizeof(memoryInfo)))
			return false;

		if (memoryInfo.State != MEM_COMMIT)
			return false;

		const auto regionBegin = reinterpret_cast<uintptr_t>(memoryInfo.BaseAddress);
		const auto regionEnd = regionBegin + memoryInfo.RegionSize;
		if (address < regionBegin)
			return false;

		if (size && (address + size < address || address + size > regionEnd))
			return false;

		return true;
	}

	std::string AddressReason(const MEMORY_BASIC_INFORMATION& memoryInfo, bool hasModule, bool expectExecutable)
	{
		if (memoryInfo.State != MEM_COMMIT)
			return "not-committed";

		if (memoryInfo.Protect & PAGE_GUARD)
			return "guard-page";

		if (BaseProtection(memoryInfo.Protect) == PAGE_NOACCESS)
			return "no-access";

		if (expectExecutable && !IsExecutableProtect(memoryInfo.Protect))
			return "not-executable";

		if (expectExecutable && memoryInfo.Type != MEM_IMAGE)
			return "not-module-image";

		if (memoryInfo.Type == MEM_IMAGE && !hasModule)
			return "image-without-module";

		return "ok";
	}

	std::string GetModuleOffset(uintptr_t address)
	{
		std::string result;
		if (TryGetModuleOffset(address, result))
			return result;

		std::ostringstream stream;
		stream << "0x" << std::hex << address;
		return stream.str();
	}

	bool ReadPointer(uintptr_t address, uintptr_t& value)
	{
		MEMORY_BASIC_INFORMATION memoryInfo = {};
		if (!VirtualQuery(reinterpret_cast<LPCVOID>(address), &memoryInfo, sizeof(memoryInfo)))
			return false;

		if (memoryInfo.State != MEM_COMMIT)
			return false;

		if (memoryInfo.Protect & (PAGE_NOACCESS | PAGE_GUARD))
			return false;

		const auto regionBegin = reinterpret_cast<uintptr_t>(memoryInfo.BaseAddress);
		const auto regionEnd = regionBegin + memoryInfo.RegionSize;
		if (address < regionBegin || address + sizeof(uintptr_t) > regionEnd)
			return false;

		__try
		{
			value = *reinterpret_cast<const uintptr_t*>(address);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			return false;
		}

		return true;
	}

	std::vector<std::pair<uintptr_t, std::string>> ScanStackForModulePointers(PCONTEXT context)
	{
		std::vector<std::pair<uintptr_t, std::string>> entries;
		if (!context || !context->Rsp)
			return entries;

		constexpr uintptr_t scanBytes = 0x800;
		for (uintptr_t offset = 0; offset < scanBytes; offset += sizeof(uintptr_t))
		{
			uintptr_t value = 0;
			if (!ReadPointer(context->Rsp + offset, value))
				break;

			std::string resolved;
			if (TryGetModuleOffset(value, resolved))
			{
				entries.emplace_back(offset, resolved);
				if (entries.size() >= 48)
					break;
			}
		}

		return entries;
	}

	const char* GetExceptionName(DWORD code)
	{
		switch (code)
		{
		case EXCEPTION_ACCESS_VIOLATION: return "ACCESS VIOLATION";
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: return "ARRAY BOUNDS EXCEEDED";
		case EXCEPTION_BREAKPOINT: return "BREAKPOINT";
		case EXCEPTION_DATATYPE_MISALIGNMENT: return "DATATYPE MISALIGNMENT";
		case EXCEPTION_FLT_DENORMAL_OPERAND: return "FLOAT DENORMAL OPERAND";
		case EXCEPTION_FLT_DIVIDE_BY_ZERO: return "FLOAT DIVIDE BY ZERO";
		case EXCEPTION_FLT_INEXACT_RESULT: return "FLOAT INEXACT RESULT";
		case EXCEPTION_FLT_INVALID_OPERATION: return "FLOAT INVALID OPERATION";
		case EXCEPTION_FLT_OVERFLOW: return "FLOAT OVERFLOW";
		case EXCEPTION_FLT_STACK_CHECK: return "FLOAT STACK CHECK";
		case EXCEPTION_FLT_UNDERFLOW: return "FLOAT UNDERFLOW";
		case EXCEPTION_ILLEGAL_INSTRUCTION: return "ILLEGAL INSTRUCTION";
		case EXCEPTION_IN_PAGE_ERROR: return "IN PAGE ERROR";
		case EXCEPTION_INT_DIVIDE_BY_ZERO: return "INTEGER DIVIDE BY ZERO";
		case EXCEPTION_INT_OVERFLOW: return "INTEGER OVERFLOW";
		case EXCEPTION_INVALID_DISPOSITION: return "INVALID DISPOSITION";
		case EXCEPTION_NONCONTINUABLE_EXCEPTION: return "NONCONTINUABLE EXCEPTION";
		case EXCEPTION_PRIV_INSTRUCTION: return "PRIVILEGED INSTRUCTION";
		case EXCEPTION_STACK_OVERFLOW: return "STACK OVERFLOW";
		case STATUS_HEAP_CORRUPTION: return "HEAP CORRUPTION";
		case STATUS_CPP_EH_EXCEPTION: return "C++ EXCEPTION";
		default: return "UNKNOWN";
		}
	}

	bool IsWorthLogging(DWORD code)
	{
		switch (code)
		{
		case DBG_PRINTEXCEPTION_C:
		case DBG_PRINTEXCEPTION_WIDE_C:
		case STATUS_CPP_EH_EXCEPTION:
			return false;
		case EXCEPTION_ACCESS_VIOLATION:
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
		case EXCEPTION_DATATYPE_MISALIGNMENT:
		case EXCEPTION_FLT_DIVIDE_BY_ZERO:
		case EXCEPTION_FLT_INVALID_OPERATION:
		case EXCEPTION_FLT_OVERFLOW:
		case EXCEPTION_ILLEGAL_INSTRUCTION:
		case EXCEPTION_IN_PAGE_ERROR:
		case EXCEPTION_INT_DIVIDE_BY_ZERO:
		case EXCEPTION_INT_OVERFLOW:
		case EXCEPTION_INVALID_DISPOSITION:
		case EXCEPTION_NONCONTINUABLE_EXCEPTION:
		case EXCEPTION_PRIV_INSTRUCTION:
		case EXCEPTION_STACK_OVERFLOW:
		case STATUS_HEAP_CORRUPTION:
			return true;
		default:
			return false;
		}
	}

	bool MarkLogged(void* address)
	{
		if (!address)
		{
			if (g_LoggedNullAddress)
				return false;

			g_LoggedNullAddress = true;
			return true;
		}

		for (const auto loggedAddress : g_LoggedAddresses)
		{
			if (loggedAddress == address)
				return false;
		}

		for (auto& loggedAddress : g_LoggedAddresses)
		{
			if (!loggedAddress)
			{
				loggedAddress = address;
				break;
			}
		}

		return true;
	}

	const char* GetAccessType(ULONG_PTR type)
	{
		switch (type)
		{
		case 0: return "read";
		case 1: return "write";
		case 8: return "execute";
		default: return "unknown";
		}
	}

	struct DbgHelpApi
	{
		HMODULE module = nullptr;
		BOOL(WINAPI* MiniDumpWriteDump)(HANDLE, DWORD, HANDLE, MINIDUMP_TYPE, PMINIDUMP_EXCEPTION_INFORMATION, PMINIDUMP_USER_STREAM_INFORMATION, PMINIDUMP_CALLBACK_INFORMATION) = nullptr;
		BOOL(WINAPI* SymInitialize)(HANDLE, PCSTR, BOOL) = nullptr;
		DWORD(WINAPI* SymSetOptions)(DWORD) = nullptr;
		BOOL(WINAPI* StackWalk64)(DWORD, HANDLE, HANDLE, LPSTACKFRAME64, PVOID, PREAD_PROCESS_MEMORY_ROUTINE64, PFUNCTION_TABLE_ACCESS_ROUTINE64, PGET_MODULE_BASE_ROUTINE64, PTRANSLATE_ADDRESS_ROUTINE64) = nullptr;
		PVOID(WINAPI* SymFunctionTableAccess64)(HANDLE, DWORD64) = nullptr;
		DWORD64(WINAPI* SymGetModuleBase64)(HANDLE, DWORD64) = nullptr;
		BOOL(WINAPI* SymGetLineFromAddr64)(HANDLE, DWORD64, PDWORD, PIMAGEHLP_LINE64) = nullptr;
		BOOL(WINAPI* SymFromAddr)(HANDLE, DWORD64, PDWORD64, PSYMBOL_INFO) = nullptr;
		BOOL(WINAPI* SymCleanup)(HANDLE) = nullptr;

		DbgHelpApi() = default;
		DbgHelpApi(const DbgHelpApi&) = delete;
		DbgHelpApi& operator=(const DbgHelpApi&) = delete;

		DbgHelpApi(DbgHelpApi&& other) noexcept
		{
			*this = std::move(other);
		}

		DbgHelpApi& operator=(DbgHelpApi&& other) noexcept
		{
			if (this == &other)
				return *this;

			module = other.module;
			MiniDumpWriteDump = other.MiniDumpWriteDump;
			SymInitialize = other.SymInitialize;
			SymSetOptions = other.SymSetOptions;
			StackWalk64 = other.StackWalk64;
			SymFunctionTableAccess64 = other.SymFunctionTableAccess64;
			SymGetModuleBase64 = other.SymGetModuleBase64;
			SymGetLineFromAddr64 = other.SymGetLineFromAddr64;
			SymFromAddr = other.SymFromAddr;
			SymCleanup = other.SymCleanup;

			other.module = nullptr;
			return *this;
		}

		~DbgHelpApi()
		{
			if (module)
				FreeLibrary(module);
		}
	};

	template <typename T>
	T GetProc(HMODULE module, const char* name)
	{
		return reinterpret_cast<T>(GetProcAddress(module, name));
	}

	DbgHelpApi LoadDbgHelp()
	{
		DbgHelpApi api;
		api.module = LoadLibraryW(L"dbghelp.dll");
		if (!api.module)
			return api;

		api.MiniDumpWriteDump = GetProc<decltype(api.MiniDumpWriteDump)>(api.module, "MiniDumpWriteDump");
		api.SymInitialize = GetProc<decltype(api.SymInitialize)>(api.module, "SymInitialize");
		api.SymSetOptions = GetProc<decltype(api.SymSetOptions)>(api.module, "SymSetOptions");
		api.StackWalk64 = GetProc<decltype(api.StackWalk64)>(api.module, "StackWalk64");
		api.SymFunctionTableAccess64 = GetProc<decltype(api.SymFunctionTableAccess64)>(api.module, "SymFunctionTableAccess64");
		api.SymGetModuleBase64 = GetProc<decltype(api.SymGetModuleBase64)>(api.module, "SymGetModuleBase64");
		api.SymGetLineFromAddr64 = GetProc<decltype(api.SymGetLineFromAddr64)>(api.module, "SymGetLineFromAddr64");
		api.SymFromAddr = GetProc<decltype(api.SymFromAddr)>(api.module, "SymFromAddr");
		api.SymCleanup = GetProc<decltype(api.SymCleanup)>(api.module, "SymCleanup");
		return api;
	}

	std::wstring BuildDumpPath()
	{
		SYSTEMTIME time = {};
		GetLocalTime(&time);
		EnsureCrashOutputDirectory();

		wchar_t fileName[MAX_PATH] = {};
		swprintf_s(
			fileName,
			L"%s\\Crash_FW_%04u%02u%02u_%02u%02u%02u_%lu_%lu.dmp",
			GetCrashOutputDirectory().c_str(),
			time.wYear,
			time.wMonth,
			time.wDay,
			time.wHour,
			time.wMinute,
			time.wSecond,
			GetCurrentProcessId(),
			GetCurrentThreadId()
		);

		return fileName;
	}

	std::wstring GetLogPath()
	{
		EnsureCrashOutputDirectory();
		return GetCrashOutputDirectory() + L"\\Fedoraware_crash_log.txt";
	}

	bool WriteMinidump(PEXCEPTION_POINTERS exceptionInfo, std::wstring& dumpPath, DWORD& error)
	{
		auto dbgHelp = LoadDbgHelp();
		if (!dbgHelp.MiniDumpWriteDump)
		{
			error = GetLastError();
			return false;
		}

		dumpPath = BuildDumpPath();
		const HANDLE file = CreateFileW(dumpPath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (file == INVALID_HANDLE_VALUE)
		{
			error = GetLastError();
			return false;
		}

		MINIDUMP_EXCEPTION_INFORMATION dumpExceptionInfo = {};
		dumpExceptionInfo.ThreadId = GetCurrentThreadId();
		dumpExceptionInfo.ExceptionPointers = exceptionInfo;
		dumpExceptionInfo.ClientPointers = FALSE;

		const auto dumpType = static_cast<MINIDUMP_TYPE>(
			MiniDumpWithDataSegs |
			MiniDumpWithIndirectlyReferencedMemory |
			MiniDumpWithThreadInfo
		);

		const BOOL written = dbgHelp.MiniDumpWriteDump(
			GetCurrentProcess(),
			GetCurrentProcessId(),
			file,
			dumpType,
			&dumpExceptionInfo,
			nullptr,
			nullptr
		);

		error = written ? ERROR_SUCCESS : GetLastError();
		CloseHandle(file);
		return written == TRUE;
	}

	void AppendLog(const std::wstring& path, const std::string& text)
	{
		const HANDLE file = CreateFileW(path.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (file == INVALID_HANDLE_VALUE)
			return;

		DWORD written = 0;
		WriteFile(file, text.data(), static_cast<DWORD>(text.size()), &written, nullptr);
		CloseHandle(file);
	}

	struct StackFrame
	{
		uintptr_t address = 0;
		uintptr_t base = 0;
		std::string module;
		std::string symbol;
		std::string file;
		DWORD line = 0;
	};

	std::vector<StackFrame> CaptureStackTrace(PCONTEXT context)
	{
		std::vector<StackFrame> frames;
		const HANDLE process = GetCurrentProcess();
		const HANDLE thread = GetCurrentThread();

		auto dbgHelp = LoadDbgHelp();
		if (!dbgHelp.SymInitialize || !dbgHelp.SymSetOptions || !dbgHelp.StackWalk64
			|| !dbgHelp.SymFunctionTableAccess64 || !dbgHelp.SymGetModuleBase64 || !dbgHelp.SymCleanup)
			return frames;

		if (!dbgHelp.SymInitialize(process, nullptr, TRUE))
			return frames;

		dbgHelp.SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);

		STACKFRAME64 stackFrame = {};
		stackFrame.AddrPC.Offset = context->Rip;
		stackFrame.AddrFrame.Offset = context->Rbp;
		stackFrame.AddrStack.Offset = context->Rsp;
		stackFrame.AddrPC.Mode = AddrModeFlat;
		stackFrame.AddrFrame.Mode = AddrModeFlat;
		stackFrame.AddrStack.Mode = AddrModeFlat;

		CONTEXT contextCopy = *context;
		while (dbgHelp.StackWalk64(IMAGE_FILE_MACHINE_AMD64, process, thread, &stackFrame, &contextCopy, nullptr, dbgHelp.SymFunctionTableAccess64, dbgHelp.SymGetModuleBase64, nullptr))
		{
			if (!stackFrame.AddrPC.Offset)
				break;

			StackFrame frame;
			frame.address = static_cast<uintptr_t>(stackFrame.AddrPC.Offset);

			if (const auto base = dbgHelp.SymGetModuleBase64(process, stackFrame.AddrPC.Offset))
			{
				frame.base = static_cast<uintptr_t>(base);
				frame.module = GetModuleName(reinterpret_cast<HMODULE>(frame.base));
			}

			DWORD displacement = 0;
			IMAGEHLP_LINE64 line = {};
			line.SizeOfStruct = sizeof(line);
			if (dbgHelp.SymGetLineFromAddr64 && dbgHelp.SymGetLineFromAddr64(process, stackFrame.AddrPC.Offset, &displacement, &line))
			{
				frame.file = GetFileName(line.FileName);
				frame.line = line.LineNumber;
			}

			std::array<char, sizeof(SYMBOL_INFO) + MAX_SYM_NAME> symbolBuffer = {};
			auto symbol = reinterpret_cast<PSYMBOL_INFO>(symbolBuffer.data());
			symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
			symbol->MaxNameLen = MAX_SYM_NAME;
			DWORD64 symbolDisplacement = 0;
			if (dbgHelp.SymFromAddr && dbgHelp.SymFromAddr(process, stackFrame.AddrPC.Offset, &symbolDisplacement, symbol))
				frame.symbol = symbol->Name;

			frames.push_back(frame);
			if (frames.size() >= 64)
				break;
		}

		dbgHelp.SymCleanup(process);
		return frames;
	}

	std::string BuildReport(PEXCEPTION_POINTERS exceptionInfo, const std::wstring& dumpPath, bool dumpWritten, DWORD dumpError, const char* source)
	{
		const auto record = exceptionInfo->ExceptionRecord;
		const auto context = exceptionInfo->ContextRecord;

		SYSTEMTIME time = {};
		GetLocalTime(&time);

		std::ostringstream stream;
		stream << "============================================================\n";
		stream << "Fedoraware crash log\n";
		stream << "Source: " << source << "\n";
		stream << "Build: " << __DATE__ << " " << __TIME__ << "\n";
		stream << "Time: "
			<< std::setfill('0') << std::setw(4) << time.wYear << "-"
			<< std::setw(2) << time.wMonth << "-"
			<< std::setw(2) << time.wDay << " "
			<< std::setw(2) << time.wHour << ":"
			<< std::setw(2) << time.wMinute << ":"
			<< std::setw(2) << time.wSecond << "\n";
		stream << std::setfill(' ');
		stream << "Module: " << GetModuleName(g_Module) << "\n";
		stream << "Exception: " << GetExceptionName(record->ExceptionCode) << " (0x" << std::hex << std::uppercase << record->ExceptionCode << std::dec << ")\n";
		stream << "Address: " << GetModuleOffset(reinterpret_cast<uintptr_t>(record->ExceptionAddress)) << "\n";
		stream << "AddressDetail: " << Minidump::DescribeAddress(reinterpret_cast<uintptr_t>(record->ExceptionAddress), true) << "\n";
		if (record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && record->NumberParameters >= 2)
		{
			stream << "Access: " << GetAccessType(record->ExceptionInformation[0]) << " 0x"
				<< std::hex << record->ExceptionInformation[1] << std::dec << "\n";
			stream << "AccessDetail: " << Minidump::DescribeAddress(static_cast<uintptr_t>(record->ExceptionInformation[1])) << "\n";
		}

		if (dumpWritten)
			stream << "Dump: " << Narrow(dumpPath) << "\n";
		else
			stream << "Dump: failed, GetLastError=" << dumpError << ", path=" << Narrow(dumpPath) << "\n";

		stream << "\nRegisters:\n";
		stream << std::hex << std::nouppercase;
		stream << "RIP=0x" << context->Rip << " RSP=0x" << context->Rsp << " RBP=0x" << context->Rbp << "\n";
		stream << "RAX=0x" << context->Rax << " RBX=0x" << context->Rbx << " RCX=0x" << context->Rcx << " RDX=0x" << context->Rdx << "\n";
		stream << "RSI=0x" << context->Rsi << " RDI=0x" << context->Rdi << " R8=0x" << context->R8 << " R9=0x" << context->R9 << "\n";
		stream << "R10=0x" << context->R10 << " R11=0x" << context->R11 << " R12=0x" << context->R12 << " R13=0x" << context->R13 << "\n";
		stream << "R14=0x" << context->R14 << " R15=0x" << context->R15 << "\n";

		stream << "\nStack trace:\n";
		const auto frames = CaptureStackTrace(context);
		if (frames.empty())
		{
			stream << "1: " << GetModuleOffset(reinterpret_cast<uintptr_t>(record->ExceptionAddress)) << "\n";
		}
		else
		{
			for (size_t i = 0; i < frames.size(); ++i)
			{
				const auto& frame = frames[i];
				stream << std::dec << i + 1 << ": ";
				if (!frame.module.empty() && frame.base)
					stream << frame.module << "+0x" << std::hex << (frame.address - frame.base);
				else
					stream << "0x" << std::hex << frame.address;

				if (!frame.symbol.empty())
					stream << " (" << frame.symbol << ")";
				if (!frame.file.empty())
					stream << " (" << frame.file << ":" << std::dec << frame.line << ")";
				stream << "\n";
			}
		}

		const auto stackEntries = ScanStackForModulePointers(context);
		if (!stackEntries.empty())
		{
			stream << "\nStack scan:\n";
			for (const auto& [offset, resolved] : stackEntries)
			{
				stream << "+0x" << std::hex << offset << ": " << resolved << "\n";
			}
		}

		stream << "\n\n";
		return stream.str();
	}

	void TraceException(PEXCEPTION_POINTERS exceptionInfo, const char* source)
	{
		if (!exceptionInfo || !exceptionInfo->ExceptionRecord)
			return;

		const auto count = InterlockedIncrement(&g_ExceptionTraceCount);
		if (count > 256)
			return;

		const auto record = exceptionInfo->ExceptionRecord;
		if (!IsWorthLogging(record->ExceptionCode))
			return;

		std::ostringstream stream;
		stream << source
			<< " code=0x" << std::hex << std::uppercase << record->ExceptionCode
			<< " " << GetExceptionName(record->ExceptionCode)
			<< " address=" << GetModuleOffset(reinterpret_cast<uintptr_t>(record->ExceptionAddress));
		if (record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && record->NumberParameters >= 2)
		{
			stream << " access=" << GetAccessType(record->ExceptionInformation[0])
				<< " target=0x" << std::hex << record->ExceptionInformation[1]
				<< " targetDetail=" << Minidump::DescribeAddress(static_cast<uintptr_t>(record->ExceptionInformation[1]));
		}

		WriteLine(GetTracePath(), stream.str().c_str());
	}

	void WriteCrashReport(PEXCEPTION_POINTERS exceptionInfo, const char* source, bool force = false)
	{
		if (!exceptionInfo || !exceptionInfo->ExceptionRecord || !exceptionInfo->ContextRecord)
			return;

		if (!force && !IsWorthLogging(exceptionInfo->ExceptionRecord->ExceptionCode))
			return;

		if (!MarkLogged(exceptionInfo->ExceptionRecord->ExceptionAddress))
			return;

		std::wstring dumpPath;
		DWORD dumpError = ERROR_SUCCESS;
		const bool dumpWritten = WriteMinidump(exceptionInfo, dumpPath, dumpError);
		const auto report = BuildReport(exceptionInfo, dumpPath, dumpWritten, dumpError, source);

		AppendLog(GetLogPath(), report);
#if FW_MINIDUMP_DIAGNOSTICS
		OutputDebugStringA(report.c_str());
#endif
	}

	LONG WINAPI VectoredExceptionFilter(PEXCEPTION_POINTERS exceptionInfo)
	{
		TraceException(exceptionInfo, "VEH");
		if (InterlockedCompareExchange(&g_FilterActive, 1, 0) != 0)
			return EXCEPTION_CONTINUE_SEARCH;

		WriteCrashReport(exceptionInfo, "vectored");

		InterlockedExchange(&g_FilterActive, 0);
		return EXCEPTION_CONTINUE_SEARCH;
	}
}

void Minidump::OpenConsole()
{
#if !FW_MINIDUMP_DIAGNOSTICS
	return;
#else
	if (g_DiagnosticsWindowStarted)
		return;

	if (!LaunchDiagnosticsWindow())
	{
		Trace("[WARN] Failed to launch external diagnostics window; file trace is still active");
	}
	else
	{
		Trace("[OK] External diagnostics window launched");
	}

	Trace("============================================================");
	Trace("Fedoraware diagnostics trace active");
	Trace("Watch for ERROR and WARN lines; full trace is in the local crashlogs folder next to the DLL");
#endif
}

void Minidump::Trace(const char* message)
{
#if !FW_MINIDUMP_DIAGNOSTICS
	(void)message;
#else
	WriteLine(GetTracePath(), message);
#endif
}

std::string Minidump::DescribeAddress(uintptr_t address, bool expectExecutable)
{
	std::ostringstream stream;
	stream << "0x" << std::hex << address;

	if (!address)
	{
		stream << " reason=null";
		return stream.str();
	}

	std::string moduleOffset;
	const bool hasModule = TryGetModuleOffset(address, moduleOffset);
	stream << " module=" << (hasModule ? moduleOffset : "<none>");

	MEMORY_BASIC_INFORMATION memoryInfo = {};
	if (!VirtualQuery(reinterpret_cast<LPCVOID>(address), &memoryInfo, sizeof(memoryInfo)))
	{
		stream << " reason=query-failed gle=" << std::dec << GetLastError();
		return stream.str();
	}

	const auto regionBegin = reinterpret_cast<uintptr_t>(memoryInfo.BaseAddress);
	stream << " region=0x" << std::hex << regionBegin << "+0x" << memoryInfo.RegionSize
		<< " state=" << StateName(memoryInfo.State)
		<< " protect=" << ProtectName(memoryInfo.Protect);

	if (memoryInfo.Protect & PAGE_GUARD)
		stream << "|PAGE_GUARD";
	if (memoryInfo.Protect & PAGE_NOCACHE)
		stream << "|PAGE_NOCACHE";
	if (memoryInfo.Protect & PAGE_WRITECOMBINE)
		stream << "|PAGE_WRITECOMBINE";

	stream << " type=" << TypeName(memoryInfo.Type)
		<< " readable=" << (IsReadableProtect(memoryInfo.Protect) ? "yes" : "no")
		<< " executable=" << (IsExecutableProtect(memoryInfo.Protect) ? "yes" : "no")
		<< " reason=" << AddressReason(memoryInfo, hasModule, expectExecutable);

	return stream.str();
}

bool Minidump::IsReadableAddress(uintptr_t address, size_t size)
{
	MEMORY_BASIC_INFORMATION memoryInfo = {};
	return QueryCommittedRange(address, size, memoryInfo) && IsReadableProtect(memoryInfo.Protect);
}

bool Minidump::IsExecutableAddress(uintptr_t address)
{
	MEMORY_BASIC_INFORMATION memoryInfo = {};
	return QueryCommittedRange(address, 1, memoryInfo) && IsExecutableProtect(memoryInfo.Protect);
}

void Minidump::TraceAddress(const char* label, uintptr_t address, bool expectExecutable)
{
#if !FW_MINIDUMP_DIAGNOSTICS
	(void)label;
	(void)address;
	(void)expectExecutable;
#else
	Trace((std::string("[ADDR] ") + (label ? label : "(null)") + " | " + DescribeAddress(address, expectExecutable)).c_str());
#endif
}

void Minidump::Initialize(HMODULE module)
{
#if !FW_MINIDUMP_DIAGNOSTICS
	g_Module = module;
	return;
#else
	Trace("Minidump::Initialize begin");
	g_Module = module;

	if (!g_ExceptionHandler)
		g_ExceptionHandler = AddVectoredExceptionHandler(1, VectoredExceptionFilter);

	SetUnhandledExceptionFilter(ExceptionFilter);
	Trace(g_ExceptionHandler ? "Minidump::Initialize installed VEH" : "Minidump::Initialize failed to install VEH");
#endif
}

void Minidump::Shutdown()
{
#if !FW_MINIDUMP_DIAGNOSTICS
	return;
#else
	Trace("Minidump::Shutdown begin");
	if (g_ExceptionHandler)
	{
		RemoveVectoredExceptionHandler(g_ExceptionHandler);
		g_ExceptionHandler = nullptr;
	}

	SetUnhandledExceptionFilter(nullptr);
#endif
}

LONG WINAPI Minidump::ExceptionFilter(PEXCEPTION_POINTERS exPtr)
{
#if !FW_MINIDUMP_DIAGNOSTICS
	(void)exPtr;
	return EXCEPTION_CONTINUE_SEARCH;
#else
	TraceException(exPtr, "UnhandledExceptionFilter");
	if (InterlockedCompareExchange(&g_FilterActive, 1, 0) == 0)
	{
		WriteCrashReport(exPtr, "unhandled", true);
		InterlockedExchange(&g_FilterActive, 0);
	}

	return EXCEPTION_EXECUTE_HANDLER;
#endif
}
