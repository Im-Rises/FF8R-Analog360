// FF8R-Analog360 - Restores 360° analog movement in FINAL FANTASY VIII Remastered
// Copyright (C) 2026 Quentin MOREL
// SPDX-License-Identifier: GPL-3.0-or-later

#include "logger.h"

#include "squall360.h"

#include <windows.h>
#include <xinput.h>

// The Windows SDK declares the XInput functions noexcept (WIN_NOEXCEPT), MinGW headers don't.
// Our definitions must match the header declarations exactly.
#ifndef WIN_NOEXCEPT
#define WIN_NOEXCEPT
#endif

static HMODULE g_realDll = nullptr;
constexpr auto CONSOLE_NAME = "FF8R-Analog360 - debug";
constexpr auto CONSOLE_PREFIX_MESSAGE = "[squall360]";
constexpr auto REAL_DLL_NAME = "\\XInput9_1_0.dll";

static FARPROC getRealFunction(const char* name)
{
    if (g_realDll == nullptr)
    {
        char path[MAX_PATH];
        GetSystemDirectoryA(path, MAX_PATH);
        lstrcatA(path, REAL_DLL_NAME);
        g_realDll = LoadLibraryA(path);
    }
    return (g_realDll != nullptr) ? GetProcAddress(g_realDll, name) : nullptr;
}

extern "C"
{
DWORD WINAPI XInputGetState(DWORD userIndex, XINPUT_STATE* state) WIN_NOEXCEPT
{
    using Fn = DWORD(WINAPI*)(DWORD, XINPUT_STATE*);
    const auto fn = reinterpret_cast<Fn>(getRealFunction("XInputGetState"));
    return fn ? fn(userIndex, state) : ERROR_DEVICE_NOT_CONNECTED;
}

DWORD WINAPI XInputSetState(DWORD userIndex, XINPUT_VIBRATION* vibration) WIN_NOEXCEPT
{
    using Fn = DWORD(WINAPI*)(DWORD, XINPUT_VIBRATION*);
    const auto fn = reinterpret_cast<Fn>(getRealFunction("XInputSetState"));
    return fn ? fn(userIndex, vibration) : ERROR_DEVICE_NOT_CONNECTED;
}

DWORD WINAPI XInputGetCapabilities(DWORD userIndex, DWORD flags, XINPUT_CAPABILITIES* capabilities) WIN_NOEXCEPT
{
    using Fn = DWORD(WINAPI*)(DWORD, DWORD, XINPUT_CAPABILITIES*);
    const auto fn = reinterpret_cast<Fn>(getRealFunction("XInputGetCapabilities"));
    return fn ? fn(userIndex, flags, capabilities) : ERROR_DEVICE_NOT_CONNECTED;
}

DWORD WINAPI XInputGetDSoundAudioDeviceGuids(DWORD userIndex, GUID* renderGuid, GUID* captureGuid)
{
    using Fn = DWORD(WINAPI*)(DWORD, GUID*, GUID*);
    const auto fn = reinterpret_cast<Fn>(getRealFunction("XInputGetDSoundAudioDeviceGuids"));
    return fn ? fn(userIndex, renderGuid, captureGuid) : ERROR_DEVICE_NOT_CONNECTED;
}
}

static DWORD WINAPI threadInitialisationMod(LPVOID /*param*/)
{
    logInit(CONSOLE_NAME, CONSOLE_PREFIX_MESSAGE);

    logPrint("FFVIII-Analog360 - Started");

    if (tryInstallSquall360Patch())
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
    switch (reason)
    {
        case DLL_PROCESS_ATTACH:
        {
            if (!isSupportedGameModuleLoaded())
            {
                break; // launcher or another process: do nothing
            }

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
