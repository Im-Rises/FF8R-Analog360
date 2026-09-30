// FF8R-Analog360 - Restores 360° analog movement in FINAL FANTASY VIII Remastered
// Copyright (C) 2026 Quentin MOREL
// SPDX-License-Identifier: GPL-3.0-or-later

#include "squall360.h"

#include "logger.h"

#include <algorithm>
#include <cstdint>
#include <windows.h>
#include <xinput.h>

static int g_lx = 0x80, g_ly = 0x80, g_rx = 0x80, g_ry = 0x80;
static HMODULE g_gameModule = nullptr;
static uintptr_t g_gameBaseAddress = 0;

// FUN_1031EB50 is the function to replace the calls to, it is the function normally called by the game to read joystick
// values.
static constexpr uint32_t RVA_GET_ANALOG = 0x31EB50; // FUN_1031EB50

// Addresses of the Calls to FUN_1031EB50 to replace
static constexpr uint32_t EFIGS_RVA_CALL_SITES[] = {
    /*
     * Field function FUN_10290770, it calls FUN_1031EB50 to check if we use analog values
     * then read the x and y analog values
     */
    0x291659, // analog test check if different of -1 (type 2)
    0x2918CB, // lX (type 2)
    0x291927, // lY (type 3)
};

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

static bool isCallToGetAnalog(const uintptr_t callAddress)
{
    const auto* bytes = reinterpret_cast<const uint8_t*>(callAddress);

    // Call Opcode check
    if (bytes[0] != 0xE8)
    {
        logPrint("RVA 0x%06X: not a CALL (byte 0x%02X)", static_cast<unsigned>(callAddress - g_gameBaseAddress), bytes[0]);
        return false;
    }

    // Address called (relative)
    const int32_t rel = *reinterpret_cast<const int32_t*>(bytes + 1);

    // Destination address of the next instruction (site + 5, the CALL instruction is 5 bytes long) + relative offset
    const uintptr_t dest = callAddress + 5 + rel;

    logPrint("RVA 0x%06X: CALL -> RVA 0x%06X", static_cast<unsigned>(callAddress - g_gameBaseAddress),
             static_cast<unsigned>(dest - g_gameBaseAddress));

    // The Call address should be FUN_1031eb50
    return dest == g_gameBaseAddress + RVA_GET_ANALOG;
}

bool tryInstallSquall360Patch(const char* appName)
{
    HMODULE module = GetModuleHandleA(appName);
    if (!module)
    {
        logPrint("Error: FFVIII game module not found");
        return false;
    }

    g_gameModule = module;
    g_gameBaseAddress = reinterpret_cast<uintptr_t>(module);

    logPrint("FFVIII game module found");

    logPrint("Verifying if patching function is possible");

    const bool canPatchAddresses =
        std::all_of(std::begin(EFIGS_RVA_CALL_SITES), std::end(EFIGS_RVA_CALL_SITES),
                    [](const uint32_t rva) -> bool { return isCallToGetAnalog(g_gameBaseAddress + rva); });

    if (!canPatchAddresses)
    {
        logPrint("Some call sites to patch were incorrect");
        return false;
    }

    logPrint("All call sites verified. Patching...");

    return true;
}
