// FF8R-Analog360 - Restores 360° analog movement in FINAL FANTASY VIII Remastered
// Copyright (C) 2026 Quentin MOREL
// SPDX-License-Identifier: GPL-3.0-or-later

#include "squall360.h"

#include <algorithm>
#include <windows.h>
#include <xinput.h>

static int g_lx = 0x80, g_ly = 0x80, g_rx = 0x80, g_ry = 0x80;
static HMODULE g_gameModule = nullptr;

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

bool tryInstallSquall360Patch(const char* appName)
{
    HMODULE m = GetModuleHandleA(appName);
    if (!m)
    {
        return false;
    }

    g_gameModule = m;


    return false;
}
