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

> **Note:** The patch is not compatible with MaKiPL's FF8_demaster. We use the same file to patch.

## The fix

The fix follows the same logic as the 2013 FFNx patch. I hook new functions to the original game analog reading
functions for the game engine to handle the analog values.

## How to build

```sh
cmake -B build -A Win32
cmake --build build --config Release
```

## Installation

You'll have to put the `dinput8.dll` file in the game root folder.

## Roadmap

- [x] Create a dll
- [x] Make it load the real dinput8.dll
- [x] Verify the call sites in FFVIII_EFIGS.dll
- [x] Inject the new analog function for field movements
- [x] Inject the new analog function for world map movements
- [x] Implement the fix for JP version
- [ ] Make it compatible with MaKiPL's FF8_demaster

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
  `FFVIII_EFIGS.dll` and `FFVIII_EFIGS_JP.dll`
- [x64dbg](https://github.com/x64dbg/x64dbg) (x32dbg) — debugger used to inspect the game at runtime

## License

This project is licensed under the GNU General Public License v3.0 — see [LICENSE](LICENSE).
