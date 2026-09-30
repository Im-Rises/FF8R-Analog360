// FF8R-Analog360 - Restores 360° analog movement in FINAL FANTASY VIII Remastered
// Copyright (C) 2026 Quentin MOREL
// SPDX-License-Identifier: GPL-3.0-or-later

#include "squall360.h"

#include "logger.h"

#include <algorithm>
#include <cstdint>
#include <windows.h>
#include <xinput.h>

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

// RVA of DAT_116cb5e0
static constexpr uint32_t RVA_EMU_STACK_PTR = 0x16CB5E0;

static uintptr_t g_gameBaseAddress = 0;

static int g_lx = 0x80, g_ly = 0x80, g_rx = 0x80, g_ry = 0x80;

static void pollPad()
{
    XINPUT_STATE xstate{};

    if (XInputGetState(0, &xstate) != ERROR_SUCCESS)
    {
        g_lx = g_ly = g_rx = g_ry = 0x80;

        // logPrint("Error: Cannot get XInput State");

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
    // Read the raw bytes of the instruction at this address
    const auto* bytes = reinterpret_cast<const uint8_t*>(callAddress);

    // Call Opcode check
    if (bytes[0] != 0xE8)
    {
        logPrint("RVA 0x%06X: not a CALL (byte 0x%02X)", static_cast<unsigned>(callAddress - g_gameBaseAddress),
                 bytes[0]);
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

static bool patchCallToGetAnalog(const uintptr_t callAddress, const uintptr_t hookAddress)
{
    // Read the raw bytes of the instruction at this address
    auto* bytes = reinterpret_cast<uint8_t*>(callAddress);

    // Make the 5 bytes of the CALL writable
    DWORD oldProtect = 0;
    if (!VirtualProtect(bytes, 5, PAGE_EXECUTE_READWRITE, &oldProtect))
    {
        return false;
    }

    // A CALL rel32 jumps to: next instruction + relative offset
    // So the new offset is: hookAddress - next instruction
    const uintptr_t nextInstruction = callAddress + 5;
    const auto newRelativeOffset = static_cast<int32_t>(hookAddress - nextInstruction);

    // Overwrite the 4 bytes of the offset (after the E8 opcode)
    *reinterpret_cast<int32_t*>(bytes + 1) = newRelativeOffset;

    // Restore the original protection and make sure the CPU sees the new code
    VirtualProtect(bytes, 5, oldProtect, &oldProtect);
    FlushInstructionCache(GetCurrentProcess(), bytes, 5);
    return true;
}

// Function that will be used to replace FUN_1031EB50 (declared like in Ghidra)
static void __cdecl analogHook(uint32_t* ctx)
{
    logPrint("Calling analog hook");

    // Fetching the data from the emulated stack
    const auto* stack = *reinterpret_cast<uint8_t**>(g_gameBaseAddress + RVA_EMU_STACK_PTR);
    const uint32_t esp = ctx[0xB];
    const uint32_t type = *reinterpret_cast<const uint32_t*>(stack + esp + 8);

    pollPad();

    switch (type)
    {
        case 0:
            ctx[0x0] = g_rx;
            break;
        case 1:
            ctx[0x0] = g_ry;
            break;
        case 2:
            ctx[0x0] = g_lx;
            break;
        case 3:
            ctx[0x0] = g_ly;
            break;
        default:
            ctx[0x0] = static_cast<uint32_t>(-1);
            break;
    }

    // Pop the fake stack pushed by the caller (emulate the RET), like the original function does
    ctx[0xB] += 4;

    // logPrint("Reading type {} - {}", type, ctx[0x0]);
}

bool tryInstallSquall360Patch(const char* appName)
{
    HMODULE module = GetModuleHandleA(appName);
    if (!module)
    {
        logPrint("Error: FFVIII game module not found");
        return false;
    }

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

    const auto hookAddress = reinterpret_cast<uintptr_t>(&analogHook);
    for (const uint32_t rva : EFIGS_RVA_CALL_SITES)
    {
        if (!patchCallToGetAnalog(g_gameBaseAddress + rva, hookAddress))
        {
            logPrint("Error: failed to patch RVA 0x%06X", static_cast<unsigned>(rva));
            return false;
        }
    }

    logPrint("Patch installed");

    return true;
}
