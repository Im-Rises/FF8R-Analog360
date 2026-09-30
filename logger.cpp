#include "logger.h"

#include <windows.h>
#include <cstdarg>
#include <cstdio>

namespace
{
constexpr int PREFIX_SIZE = 32;
constexpr int MESSAGE_SIZE = 512;
constexpr int LINE_SIZE = PREFIX_SIZE + 1 + MESSAGE_SIZE + 2; // préfixe + ' ' + message + '\n' + '\0'

char g_prefix[PREFIX_SIZE]{};
}

void logInit(const char* consoleName, const char* prefix)
{
    lstrcpynA(g_prefix, (prefix != nullptr) ? prefix : "", PREFIX_SIZE);

#ifndef NDEBUG
    if (!AllocConsole())
    {
        return;
    }

    SetConsoleTitleA(consoleName);

    if (HWND console = GetConsoleWindow())
    {
        DeleteMenu(GetSystemMenu(console, FALSE), SC_CLOSE, MF_BYCOMMAND);
    }
#else
    (void)consoleName;
#endif
}

void logStop()
{
#ifndef NDEBUG
    FreeConsole();
#endif
}

void logPrint(const char* fmt, ...)
{
    char message[MESSAGE_SIZE]{};

    va_list args{};
    va_start(args, fmt);
    (void)vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);

    char line[LINE_SIZE]{};
    (void)snprintf(line, sizeof(line), "%s %s\n", g_prefix, message);

    OutputDebugStringA(line);

#ifndef NDEBUG
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    if (out != nullptr && out != INVALID_HANDLE_VALUE)
    {
        DWORD written = 0;
        WriteConsoleA(out, line, static_cast<DWORD>(lstrlenA(line)), &written, nullptr);
    }
#endif
}
