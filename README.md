# FF8R-Analog360

FFVIII Remastered to restore full 360° analog movement — a port of FFNx's Analog 360 patch to FFVIII Remastered

## The remaster

FFVIII Remastered is a new version of FFVIII adding new models, etc...

The remaster uses the base C function of the 2013, meaning all the functions aren't touch it just hook them differently
to change textures, etc...
This means that the function handling analog joystick value like in the 2013 version is still in the code.
The 2013 uses digital values to represent the movement even through you use an analog stick. The Fix made by FFNx team
was inject code in the functions reading normally the analog value.

For this patch I found the same function but for the Remastered. This patch is a "proper" way to
patch the game (instead of hex modification of the FFVIII_efigs.dll file).

## The fix

In the 2013 version of the game, the fix was found in FUN_ , in remastered the function is FUN_ .


## Goals

- [x] Added base project files
- [x] Create a dll
- [ ] Make it load the real d8input.dll
- [ ] Try modifying a FUN_ function I know what is doing
- [ ] Implement the new analog function
- [ ] Inject the new analog function with the custom d8input.dll
- [ ] Test the game...
- [ ] Iterate...

