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
     * The RVAs below are calls to FUN_1031EB50, the first call check if we use analog values and the others
     * read the x and y values of the joysticks.
     */

    // Field function FUN_10290770
    0x291659, // analog test check if different of -1 (type 2)
    0x2918CB, // lX (type 2)
    0x291927, // lY (type 3)
    // World map (FUN_10929600)
    0x929854, // lX (type 2)
    0x9298A3, // lY (type 3)
    0x9298F2, // rX (type 0)
    0x929944, // rY (type 1)
};

// RVA of the first Call FUN_1031EB50 in the world map function FUN_10929600
static constexpr uint32_t RVA_WM_FIRST_CALL_SITE = 0x929854;

// RVA of the pointer to the emulated stack buffer
static constexpr uint32_t RVA_EMU_STACK_PTR = 0x16CB5E0;

// Table of 4 KB pages mapping the emulated 2013 memory (DAT_1188edd0)
static constexpr uint32_t RVA_EMU_PAGES = 0x188EDD0;

/*
 * Emulated addresses (2013 addresses, not RVAs!)
 * These are absolute addresses but from the 2013 version. The remastered simulate the addresses of the 2013 and still
 * use the real addresses that were used from the 2013 version.
 *
 */
// Array of the inputs values from the current and previous frame (depending on the index).
static constexpr uint32_t EMU_WM_PAD_BUFFERS = 0x0203FDE8;
// The game stores two frame of button and toggle between them to always
// store the values of the buttons from the previous frame. The index help us identify which buffer (0 or 1) from
// EMU_WM_PAD_BUFFERS we should read
static constexpr uint32_t EMU_WM_PAD_BUFFER_INDEX = 0x020409BC;

static uintptr_t g_gameBaseAddress = 0;

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

    // Circular dead zone of radius 40 (same as FFNx): small stick movements are treated as "centered"
    if (((g_lx - 0x80) * (g_lx - 0x80)) + ((g_ly - 0x80) * (g_ly - 0x80)) < 1600)
    {
        g_lx = g_ly = 0x80;
    }
    if (((g_rx - 0x80) * (g_rx - 0x80)) + ((g_ry - 0x80) * (g_ry - 0x80)) < 1600)
    {
        g_rx = g_ry = 0x80;
    }
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
    // Fetching the data from the emulated stack
    const auto* stack = *reinterpret_cast<uint8_t**>(g_gameBaseAddress + RVA_EMU_STACK_PTR);
    const uint32_t esp = ctx[0xB];
    const uint32_t type = *reinterpret_cast<const uint32_t*>(stack + esp + 8);

    switch (type)
    {
        case 0:
            ctx[0x0] = g_rx;
            break;
        case 1:
            ctx[0x0] = g_ry;
            break;
        case 2:
            // type 2 is always the first value to be read for field and world map.
            // We only read the Xinput values one time this way.
            pollPad();
            ctx[0x0] = g_lx;
            break;
        case 3:
            ctx[0x0] = g_ly;
            break;
        default:
            ctx[0x0] = static_cast<uint32_t>(-1);
            break;
    }

    // Pop the fake return slot pushed by the caller (emulates the RET), like the original function does
    // like the original function does
    ctx[0xB] += 4;
}

static uint8_t* convertEmulatedAddressToRealAddress(const uint32_t emuAddress)
{
    /*
     * This function is a bit tricky on what it's doing.
     * The Remastered emulate the memory of the 2013 version, so it store in a page a lot of data, including our inputs.
     * This function converts an emulated address from the 2013 version like EMU_WM_PAD_BUFFERS and
     * EMU_WM_PAD_BUFFER_INDEX to a real address in the memory of the remastered.
     */

    auto* const* pages = reinterpret_cast<uint8_t* const*>(g_gameBaseAddress + RVA_EMU_PAGES);
    uint8_t* page = pages[emuAddress >> 12];
    return page != nullptr ? page + (emuAddress & 0xFFF) : nullptr;
}

static void __cdecl analogHookWorldMap(uint32_t* ctx)
{
    analogHook(ctx); // Normal work we fetch the analog values.

    if (g_lx == 0x80 && g_ly == 0x80)
    {
        return; // joystick not used, we use the d-pad values.
    }

    // We have analog values, so we reset the D-Pad values to not have any conflicts.

    // We get the index of the frame inputs we're on
    const auto* index = reinterpret_cast<const int16_t*>(convertEmulatedAddressToRealAddress(EMU_WM_PAD_BUFFER_INDEX));

    // Protection, index should always be 0 or 1
    if (index == nullptr || (*index != 0 && *index != 1))
    {
        return;
    }

    // We get the pointer to the "current frame" input buffer
    auto* buttonsState =
        reinterpret_cast<uint32_t*>(convertEmulatedAddressToRealAddress(EMU_WM_PAD_BUFFERS + (*index * 4)));

    if (buttonsState != nullptr)
    {
        *buttonsState &= 0x0FFF; // clear the 4 D-pad bits (0xF000)
    }
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

    for (const uint32_t rva : EFIGS_RVA_CALL_SITES)
    {
        // Selection of the hook function to inject.
        // For World Map the analog function is in conflict with the D-Pad. We need to reset the D-Pad values if
        // we use the joystick. (Same logic as FFNx mod)
        const uintptr_t hookAddress = (rva == RVA_WM_FIRST_CALL_SITE) ? reinterpret_cast<uintptr_t>(&analogHookWorldMap)
                                                                      : reinterpret_cast<uintptr_t>(&analogHook);

        if (!patchCallToGetAnalog(g_gameBaseAddress + rva, hookAddress))
        {
            logPrint("Error: failed to patch RVA 0x%06X", static_cast<unsigned>(rva));
            return false;
        }
    }

    logPrint("Patch installed");

    return true;
}
