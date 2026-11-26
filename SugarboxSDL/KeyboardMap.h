#pragma once

#include <SDL3/SDL_scancode.h>

// The emulated keyboard reads the host keys as the raw codes of Keyboards/101_keyboard_* : the PC
// scan codes (set 1, 0x100 for the extended keys) on Windows, the X11 keycodes on Linux. SDL gives
// its own scancodes (USB HID) : they are translated to the codes of the host.
// Returns 0 for a key the emulation does not know.
unsigned int SdlScancodeToHost(SDL_Scancode scancode);
