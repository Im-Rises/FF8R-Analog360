// FF8R-Analog360 - Restores 360° analog movement in FINAL FANTASY VIII Remastered
// Copyright (C) 2026 Quentin MOREL
// SPDX-License-Identifier: GPL-3.0-or-later

#include "squall360.h"

#include "logger.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <Windows.h>
#include <Xinput.h>

struct GameVersionOffsets
{
    // The name of the module to patch, it is used to get the base address of the module in memory.
    const char* moduleName;

    // The function to replace the calls to, it is the function normally called by the game
    // to read joystick values.
    uint32_t rvaGetAnalog;
    // RVA of the pointer to the emulated stack buffer
    uint32_t rvaEmuStackPtr;
    // Table of 4 KB pages mapping the emulated 2013 memory
    uint32_t rvaEmuPages;
    // RVA of the first CALL getAnalog in the world map function
    uint32_t rvaWorldMapFirstCallSite;

    /*
     * Emulated addresses (2013 addresses, not RVAs!)
     * These are absolute addresses but from the 2013 version. The remastered simulates the addresses of the 2013 and
     * still use the real addresses that were used from the 2013 version. We use it to get the d-pad values from the
     * world map function and reset them if we use the joystick to avoid conflicts.
     */
    // Array of the inputs values from the current and previous frame (depending on the index).
    uint32_t emuWorldMapPadBuffers;
    // The game stores two frames of button and toggle between them to always
    // store the values of the buttons from the previous frame. The index helps us identify which buffer (0 or 1) from
    // emuWorldMapPadBuffers we should read
    uint32_t emuWorldMapPadBufferIndex;

    /*
     * The RVAs Call Sites are calls to the analog function (rvaGetAnalog) to get the value of the joystick
     */
    std::array<uint32_t, 7> rvaCallSites;
};

static constexpr const char* APP_NAME_EFIGS = "FFVIII_EFIGS.dll";
static constexpr const char* APP_NAME_JP = "FFVIII_JP.dll";

static constexpr GameVersionOffsets EFIGS_OFFSETS = {
    .moduleName = APP_NAME_EFIGS,
    .rvaGetAnalog = 0x31EB50,
    .rvaEmuStackPtr = 0x16CB5E0,
    .rvaEmuPages = 0x188EDD0,
    .rvaWorldMapFirstCallSite = 0x929854,
    .emuWorldMapPadBuffers = 0x0203FDE8,
    .emuWorldMapPadBufferIndex = 0x020409BC,
    .rvaCallSites =
        {
            // Field function FUN_10290770
            0x291659, // analog test (type 2)
            0x2918CB, // lX (type 2)
            0x291927, // lY (type 3)

            // World map (FUN_10929600)
            0x929854, // lX (type 2)
            0x9298A3, // lY (type 3)
            0x9298F2, // rX (type 0)
            0x929944, // rY (type 1)
        },
};

// Thanks to Ghidra Version Tracking!
static constexpr GameVersionOffsets JP_OFFSETS = {
    .moduleName = APP_NAME_JP,
    .rvaGetAnalog = 0x333550,
    .rvaEmuStackPtr = 0x16DB880,
    .rvaEmuPages = 0x18A09A0,
    .rvaWorldMapFirstCallSite = 0x962CED,
    .emuWorldMapPadBuffers = 0x02543EC8,
    .emuWorldMapPadBufferIndex = 0x02544A9C,
    .rvaCallSites =
        {
            // Field (FUN_1029c400)
            0x29D1B0, // analog test (type 2)
            0x29D43F, // lX (type 2)
            0x29D49B, // lY (type 3)

            // World map (FUN_10962ab0), port 0
            0x962CED, // lX (type 2)
            0x962D3C, // lY (type 3)
            0x962D8B, // rX (type 0)
            0x962DDD, // rY (type 1)
        },
};

// All supported versions, tried in this order
static constexpr std::array SUPPORTED_VERSIONS = {&EFIGS_OFFSETS, &JP_OFFSETS};

static const GameVersionOffsets* g_offsets = nullptr;
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

    // The Call address should be FUN_1031eb50 for EFIGS and FUN_10333550 for JP
    return dest == g_gameBaseAddress + g_offsets->rvaGetAnalog;
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

// Function that will be used to replace the CALL analog function of the game (declared like in Ghidra)
static void __cdecl analogHook(uint32_t* ctx)
{
    // Fetching the data from the emulated stack
    const auto* stack = *reinterpret_cast<uint8_t**>(g_gameBaseAddress + g_offsets->rvaEmuStackPtr);
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
    ctx[0xB] += 4;
}

static uint8_t* convertEmulatedAddressToRealAddress(const uint32_t emuAddress)
{
    /*
     * This function is a bit tricky on what it's doing.
     * The Remastered emulates the memory of the 2013 version, so it stores in a page a lot of data, including our
     * inputs. This function converts an emulated address from the 2013 version like emuWorldMapPadBuffers and
     * emuWorldMapPadBufferIndex to a real address in the memory of the remastered.
     */

    auto* const* pages = reinterpret_cast<uint8_t* const*>(g_gameBaseAddress + g_offsets->rvaEmuPages);
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
    const auto* index =
        reinterpret_cast<const int16_t*>(convertEmulatedAddressToRealAddress(g_offsets->emuWorldMapPadBufferIndex));

    // Protection, index should always be 0 or 1
    if (index == nullptr || (*index != 0 && *index != 1))
    {
        return;
    }

    // We get the pointer to the "current frame" input buffer
    auto* buttonsState = reinterpret_cast<uint32_t*>(
        convertEmulatedAddressToRealAddress(g_offsets->emuWorldMapPadBuffers + (*index * 4)));

    if (buttonsState != nullptr)
    {
        *buttonsState &= 0x0FFF; // clear the 4 D-pad bits (0xF000)
    }
}

bool isSupportedGameModuleLoaded()
{
    return std::ranges::any_of(SUPPORTED_VERSIONS, [](const GameVersionOffsets* version)
                               { return GetModuleHandleA(version->moduleName) != nullptr; });
}

bool tryInstallSquall360Patch()
{
    // Find which supported version of the game is loaded
    for (const GameVersionOffsets* version : SUPPORTED_VERSIONS)
    {
        if (HMODULE module = GetModuleHandleA(version->moduleName))
        {
            g_offsets = version;
            g_gameBaseAddress = reinterpret_cast<uintptr_t>(module);
            break;
        }
    }

    if (g_offsets == nullptr)
    {
        logPrint("Error: no supported FFVIII module found");
        return false;
    }

    logPrint("%s detected", g_offsets->moduleName);
    logPrint("Verifying call sites...");

    const bool canPatchAddresses =
        std::all_of(std::begin(g_offsets->rvaCallSites), std::end(g_offsets->rvaCallSites),
                    [](const uint32_t rva) -> bool { return isCallToGetAnalog(g_gameBaseAddress + rva); });

    if (!canPatchAddresses)
    {
        logPrint("Some call sites to patch were incorrect");
        return false;
    }

    logPrint("All call sites verified. Patching...");

    for (const uint32_t rva : g_offsets->rvaCallSites)
    {
        // Selection of the hook function to inject.
        // For World Map the analog function is in conflict with the D-Pad. We need to reset the D-Pad values if
        // we use the joystick. (Same logic as FFNx mod)
        const uintptr_t hookAddress = (rva == g_offsets->rvaWorldMapFirstCallSite)
                                          ? reinterpret_cast<uintptr_t>(&analogHookWorldMap)
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
