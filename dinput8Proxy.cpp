#include <windows.h>

static HMODULE g_realDll = nullptr;

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

BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        // put call to injetion here
    }

    return TRUE;
}
