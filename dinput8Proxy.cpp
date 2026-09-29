#include "logger.h"

#include <cstring>
#include <windows.h>

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
        auto fn = (Fn)getRealFunction("DirectInput8Create");
        return fn ? fn(hinst, ver, riid, out, outer) : E_FAIL;
    }

    HRESULT WINAPI DllCanUnloadNow()
    {
        using Fn = HRESULT(WINAPI*)();
        auto fn = (Fn)getRealFunction("DllCanUnloadNow");
        return fn ? fn() : S_FALSE;
    }

    HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID riid, LPVOID* out)
    {
        using Fn = HRESULT(WINAPI*)(REFCLSID, REFIID, LPVOID*);
        auto fn = (Fn)getRealFunction("DllGetClassObject");
        return fn ? fn(clsid, riid, out) : E_FAIL;
    }

    HRESULT WINAPI DllRegisterServer()
    {
        using Fn = HRESULT(WINAPI*)();
        auto fn = (Fn)getRealFunction("DllRegisterServer");
        return fn ? fn() : E_FAIL;
    }

    HRESULT WINAPI DllUnregisterServer()
    {
        using Fn = HRESULT(WINAPI*)();
        auto fn = (Fn)getRealFunction("DllUnregisterServer");
        return fn ? fn() : E_FAIL;
    }
}

static bool isGameProcess()
{
    return GetModuleHandleA(APP_NAME_EFIGS) != nullptr
        || GetModuleHandleA(APP_NAME_JP) != nullptr;
}

BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (isGameProcess())
        {
            logInit(CONSOLE_NAME, CONSOLE_PREFIX_MESSAGE);

            logPrint("FFVIII-Analog360 - Started");

            logPrint("FFVIII-Analog360 - Patched");
        }
    }

    return TRUE;
}
