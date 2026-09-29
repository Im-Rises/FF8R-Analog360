#include <algorithm>
#include <windows.h>
#include <xinput.h>

static int g_lx = 0x80, g_ly = 0x80, g_rx = 0x80, g_ry = 0x80;

static void pollPad()
{
    XINPUT_STATE xstate{};

    if (XInputGetState(0, &xstate) != ERROR_SUCCESS)
    {
        g_lx = g_ly = g_rx = g_ry = 0x80;
        return;
    }

    g_lx = std::clamp(0x80 + (xstate.Gamepad.sThumbLX >> 8), 0x00, 0xFF);
    g_ly = std::clamp(0x80 - (xstate.Gamepad.sThumbLY >> 8), 0x00, 0xFF);
    g_rx = std::clamp(0x80 + (xstate.Gamepad.sThumbRX >> 8), 0x00, 0xFF);
    g_ry = std::clamp(0x80 - (xstate.Gamepad.sThumbRY >> 8), 0x00, 0xFF);

    // Handle dead zone like FFNx
}
