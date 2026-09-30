# FF8R-Analog360

A mod for FFVIII Remastered that enables full 360° analog movement, based on FFNx’s Analog 360° patch.

[![Build](https://github.com/Im-Rises/FF8R-Analog360/actions/workflows/build.yml/badge.svg)](https://github.com/Im-Rises/FF8R-Analog360/actions/workflows/build.yml)

## Context

FFVIII Remastered (2019) is a remaster of the PC version with new character models and HD assets.

The remastered is based on the 2013 version which is the port of the original PS1 code of the game. The analog values of the game aren't read as
the function which is supposed to read them returns -1, meaning we're not using a joystick.
So by default the game will read only the d-pad values.
The Remastered has the same problem, the same code still exists, but it was translated from x86 to C and is now in `FFVIII_EFIGS.dll`.

This project is a port of the patch from the 2013 version to the remastered.

## The fix

The fix follows the same logic as the 2013 FFNx patch. I hook new functions to the original game analog reading functions for the game engine to handle the analog values.

## How to build

```sh
cmake -B build -A Win32
cmake --build build --config Release
```

## Installation

You'll have to put the `dinput8.dll` file in the game root folder.

## Roadmap

- [x] Added base project files
- [x] Create a dll
- [x] Make it load the real dinput8.dll
- [ ] Verify the call sites in FFVIII_EFIGS.dll
- [ ] Implement the new analog function
- [ ] Inject the new analog function with the custom dinput8.dll
- [ ] Test the game...
- [ ] Iterate...
- [ ] Implement the fix for JP version

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
- [Ghidra](https://github.com/NationalSecurityAgency/ghidra) — reverse engineering tool used to analyse `FFVIII_EFIGS.dll`
- [x64dbg](https://github.com/x64dbg/x64dbg) (x32dbg) — debugger used to inspect the game at runtime

## License

This project is licensed under the GNU General Public License v3.0 — see [LICENSE](LICENSE).
