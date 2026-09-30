// FF8R-Analog360 - Restores 360° analog movement in FINAL FANTASY VIII Remastered
// Copyright (C) 2026 Quentin MOREL
// SPDX-License-Identifier: GPL-3.0-or-later

#include "logger.h"

#include "squall360.h"

#include <windows.h>
#include <dinput.h>

static HMODULE g_realDll = nullptr;
constexpr auto APP_NAME_EFIGS = "FFVIII_EFIGS.dll";
constexpr auto APP_NAME_JP = "FFVIII_JP.dll";
constexpr auto CONSOLE_NAME = "FF8R-Analog360 - debug";
constexpr auto CONSOLE_PREFIX_MESSAGE = "[squall360]";

static FARPROC getRealFunction(const char* name)
{
    if (g_realDll == nullptr)
    {
        char path[MAX_PATH];
        GetSystemDirectoryA(path, MAX_PATH);
        lstrcatA(path, "\\dinput8.dll");
        g_realDll = LoadLibraryA(path);
    }
    return (g_realDll != nullptr) ? GetProcAddress(g_realDll, name) : nullptr;
}

extern "C"
{
HRESULT WINAPI DirectInput8Create(HINSTANCE hinst, DWORD ver, REFIID riid, LPVOID* out, LPUNKNOWN outer)
{
    using Fn = HRESULT(WINAPI*)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);
    const auto fn = reinterpret_cast<Fn>(getRealFunction("DirectInput8Create"));
    return fn ? fn(hinst, ver, riid, out, outer) : E_FAIL;
}

HRESULT WINAPI DllCanUnloadNow()
{
    using Fn = HRESULT(WINAPI*)();
    const auto fn = reinterpret_cast<Fn>(getRealFunction("DllCanUnloadNow"));
    return fn ? fn() : S_FALSE;
}

HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID riid, LPVOID* out)
{
    using Fn = HRESULT(WINAPI*)(REFCLSID, REFIID, LPVOID*);
    const auto fn = reinterpret_cast<Fn>(getRealFunction("DllGetClassObject"));
    return fn ? fn(clsid, riid, out) : E_FAIL;
}

HRESULT WINAPI DllRegisterServer()
{
    using Fn = HRESULT(WINAPI*)();
    const auto fn = reinterpret_cast<Fn>(getRealFunction("DllRegisterServer"));
    return fn ? fn() : E_FAIL;
}

HRESULT WINAPI DllUnregisterServer()
{
    using Fn = HRESULT(WINAPI*)();
    const auto fn = reinterpret_cast<Fn>(getRealFunction("DllUnregisterServer"));
    return fn ? fn() : E_FAIL;
}

LPCDIDATAFORMAT WINAPI GetdfDIJoystick()
{
    using Fn = LPCDIDATAFORMAT(WINAPI*)();
    const auto fn = reinterpret_cast<Fn>(getRealFunction("GetdfDIJoystick"));
    return fn ? fn() : nullptr;
}
}

static bool isGameProcess()
{
    return GetModuleHandleA(APP_NAME_EFIGS) != nullptr /*|| GetModuleHandleA(APP_NAME_JP) != nullptr*/;
}

static DWORD WINAPI threadInitialisationMod(LPVOID /*param*/)
{
    logInit(CONSOLE_NAME, CONSOLE_PREFIX_MESSAGE);

    logPrint("FFVIII-Analog360 - Started");

    if (tryInstallSquall360Patch(APP_NAME_EFIGS))
    {
        logPrint("Success patching");
    }
    else
    {
        logPrint("Failure patching");
    }

    return 0;
}

BOOL WINAPI DllMain(HINSTANCE /*inst*/, DWORD reason, LPVOID /*reserved*/)
{
    if (!isGameProcess())
    {
        return TRUE;
    }

    switch (reason)
    {
        case DLL_PROCESS_ATTACH:
        {
            if (HANDLE thread = CreateThread(nullptr, 0, threadInitialisationMod, nullptr, 0, nullptr))
            {
                CloseHandle(thread);
            }

            break;
        }

        case DLL_PROCESS_DETACH:
        {
            logStop();

            break;
        }
        default:
        {
            break;
        }
    }

    return TRUE;
}
