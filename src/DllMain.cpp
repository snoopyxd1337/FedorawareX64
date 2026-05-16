#include <Windows.h>
#include <string>
#include <vector>
#include "Core/Core.h"
#include "Utils/Minidump/Minidump.h"

static bool WaitForGameModules()
{
	const std::vector<LPCWSTR> modules = {
		L"client.dll",
		L"engine.dll",
		L"inputsystem.dll",
		L"materialsystem.dll",
		L"shaderapidx9.dll",
		L"studiorender.dll",
		L"tier0.dll",
		L"vguimatsurface.dll",
		L"vgui2.dll",
		L"vphysics.dll",
		L"vstdlib.dll"
	};

	const DWORD start = GetTickCount();
	while (true)
	{
		std::wstring missing;
		for (const auto module : modules)
		{
			if (!GetModuleHandleW(module))
			{
				missing += L"\n- ";
				missing += module;
			}
		}

		if (missing.empty())
		{
			return true;
		}

		if (GetTickCount() - start > 60000)
		{
			MessageBoxW(nullptr, (L"Timed out waiting for TF2 modules:" + missing).c_str(), L"Fedoraware load timeout", MB_OK | MB_ICONERROR);
			return false;
		}

		Sleep(250);
	}
}

DWORD WINAPI MainThread(LPVOID lpParam)
{
	Minidump::OpenConsole();
	FW_TRACE("MainThread start");

	if (!WaitForGameModules())
	{
		FW_TRACE("WaitForGameModules failed");
		Minidump::Shutdown();
		FreeLibraryAndExitThread(static_cast<HMODULE>(lpParam), EXIT_FAILURE);
	}

	FW_TRACE("WaitForGameModules ok; g_Core.Load begin");
	g_Core.Load();
	FW_TRACE("g_Core.Load returned");

	DWORD lastHeartbeat = GetTickCount();
	FW_TRACE("MainThread entering unload wait loop");
	while (!g_Core.ShouldUnload())
	{
		const DWORD now = GetTickCount();
		if (now - lastHeartbeat >= 1000)
		{
			FW_TRACE("MainThread heartbeat");
			lastHeartbeat = now;
		}

		Sleep(20);
	}

	g_Core.Unload();

	FW_TRACE("MainThread unload");
	Minidump::Shutdown();
	FreeLibraryAndExitThread(static_cast<HMODULE>(lpParam), EXIT_SUCCESS);
}


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		FW_TRACE("DllMain DLL_PROCESS_ATTACH");
		Minidump::Initialize(hinstDLL);

		if (const auto hMainThread = CreateThread(nullptr, 0, MainThread, hinstDLL, 0, nullptr))
		{
			FW_TRACE("MainThread created");
			CloseHandle(hMainThread);
		}
		else
		{
			FW_TRACE("CreateThread failed");
		}
	}
	else if (fdwReason == DLL_PROCESS_DETACH)
	{
		FW_TRACE("DllMain DLL_PROCESS_DETACH");
		Minidump::Shutdown();
	}

	return TRUE;
}
