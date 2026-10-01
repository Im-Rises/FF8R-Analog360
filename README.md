# FF8R-Analog360

A mod for FFVIII Remastered that enables full 360° analog movement, based on FFNx’s Analog 360° patch.

[![Build](https://github.com/Im-Rises/FF8R-Analog360/actions/workflows/build.yml/badge.svg)](https://github.com/Im-Rises/FF8R-Analog360/actions/workflows/build.yml)
[![Release](https://github.com/Im-Rises/FF8R-Analog360/actions/workflows/release.yml/badge.svg)](https://github.com/Im-Rises/FF8R-Analog360/actions/workflows/release.yml)

## Context

FFVIII Remastered (2019) is a remaster of the PC version with new character models and HD assets.

The Remastered is based on the 2013 PC version, itself a port of the original PlayStation game. The engine still
contains the PlayStation analog movement code, but on PC the function that reads the analog stick always returns -1 ("no
analog controller"), so that code is never used. Instead, the stick is converted into D-pad inputs, which is why
characters can only move in 8 directions, always at full speed.
The Remastered has the same problem, the same code still exists, but it was translated from x86 to C and is now in
`FFVIII_EFIGS.dll`.

This project is a port of the patch from the 2013 version to the remastered.

## The fix

The fix follows the same logic as the 2013 FFNx patch. I hook new functions to the original game analog reading
functions for the game engine to handle the analog values.

## How to build

```sh
cmake -B build -A Win32
cmake --build build --config Release
```

## Installation

1. Download `xinput9_1_0.dll` from the [latest release](https://github.com/Im-Rises/FF8R-Analog360/releases/latest).
2. Copy it into the game root folder (next to `FFVIII.exe`).

**Upgrading from 1.x:** delete the old `dinput8.dll` from the game folder.

**GOG version:** the game folder already contains a `xinput9_1_0.dll` (GOG.com Input wrapper).
GOG support is planned for 2.1. If you install the mod now, back up the original file first;
replacing it may disable GOG's extra controller support.

## Compatibility

- Steam: EN/FR/DE/IT/ES (`FFVIII_EFIGS.dll`) and JP (`FFVIII_JP.dll`)
- GOG: works (replaces the GOG.com Input wrapper, see Installation)
- Compatible with MaKiPL's [FF8_demaster](https://github.com/MaKiPL/FF8_demaster)

## Roadmap

- [x] Create a dll
- [x] Make the game load the real .dll file
- [x] Verify the call sites in FFVIII_EFIGS.dll
- [x] Inject the new analog function for field movements
- [x] Inject the new analog function for world map movements
- [x] Implement the fix for JP version
- [x] Make it compatible with MaKiPL's FF8_demaster
- [ ] Check what the GOG custom xinput9_1_0.dll does exaclty (I tested it worked but it may disable some features)

## Contributors

Quentin MOREL:

- @Im-Rises
- <https://github.com/Im-Rises>

[![GitHub contributors](https://contrib.rocks/image?repo=Im-Rises/FF8R-Analog360)](https://github.com/Im-Rises/FF8R-Analog360/graphs/contributors)

## Credits

- **[FFNx](https://github.com/julianxhokaxhiu/FFNx)** by Julian Xhokaxhiu and contributors —
  this project is a port of FFNx's *Analog 360* patch for the 2013 PC version of FINAL FANTASY VIII.

## Documentation

- [FFVIII Demastered](https://github.com/MaKiPL/FF8_demaster) — another FFVIII Remastered mod, useful reference
- [Ghidra](https://github.com/NationalSecurityAgency/ghidra) — reverse engineering tool used to analyse
  `FFVIII_EFIGS.dll` and `FFVIII_JP.dll`
- [x64dbg](https://github.com/x64dbg/x64dbg) (x32dbg) — debugger used to inspect the game at runtime

## License

This project is licensed under the GNU General Public License v3.0 — see [LICENSE](LICENSE).
