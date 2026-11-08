#include "KeyboardMap.h"

// PC scan code, set 1. Extended keys (E0 prefix) carry 0x100, as Qt reports them on Windows.
static unsigned int ToPcScanCode(SDL_Scancode scancode)
{
   if (scancode >= SDL_SCANCODE_A && scancode <= SDL_SCANCODE_Z)
   {
      static const unsigned char letters[26] = {
         0x1E, 0x30, 0x2E, 0x20, 0x12, 0x21, 0x22, 0x23, 0x17, 0x24, 0x25, 0x26, 0x32, // A..M
         0x31, 0x18, 0x19, 0x10, 0x13, 0x1F, 0x14, 0x16, 0x2F, 0x11, 0x2D, 0x15, 0x2C  // N..Z
      };
      return letters[scancode - SDL_SCANCODE_A];
   }
   if (scancode >= SDL_SCANCODE_1 && scancode <= SDL_SCANCODE_0)
   {
      // 1..9 then 0 : 0x02..0x0B
      return 0x02 + (scancode - SDL_SCANCODE_1);
   }
   if (scancode >= SDL_SCANCODE_F1 && scancode <= SDL_SCANCODE_F10)
   {
      return 0x3B + (scancode - SDL_SCANCODE_F1);
   }

   switch (scancode)
   {
   case SDL_SCANCODE_RETURN:       return 0x1C;
   case SDL_SCANCODE_ESCAPE:       return 0x01;
   case SDL_SCANCODE_BACKSPACE:    return 0x0E;
   case SDL_SCANCODE_TAB:          return 0x0F;
   case SDL_SCANCODE_SPACE:        return 0x39;
   case SDL_SCANCODE_MINUS:        return 0x0C;
   case SDL_SCANCODE_EQUALS:       return 0x0D;
   case SDL_SCANCODE_LEFTBRACKET:  return 0x1A;
   case SDL_SCANCODE_RIGHTBRACKET: return 0x1B;
   case SDL_SCANCODE_BACKSLASH:    return 0x2B;
   case SDL_SCANCODE_NONUSHASH:    return 0x2B;
   case SDL_SCANCODE_SEMICOLON:    return 0x27;
   case SDL_SCANCODE_APOSTROPHE:   return 0x28;
   case SDL_SCANCODE_GRAVE:        return 0x29;
   case SDL_SCANCODE_COMMA:        return 0x33;
   case SDL_SCANCODE_PERIOD:       return 0x34;
   case SDL_SCANCODE_SLASH:        return 0x35;
   case SDL_SCANCODE_CAPSLOCK:     return 0x3A;
   case SDL_SCANCODE_F11:          return 0x57;
   case SDL_SCANCODE_F12:          return 0x58;
   case SDL_SCANCODE_SCROLLLOCK:   return 0x46;
   case SDL_SCANCODE_NUMLOCKCLEAR: return 0x45;
   case SDL_SCANCODE_NONUSBACKSLASH: return 0x56;
   case SDL_SCANCODE_LCTRL:        return 0x1D;
   case SDL_SCANCODE_LSHIFT:       return 0x2A;
   case SDL_SCANCODE_LALT:         return 0x38;
   case SDL_SCANCODE_RSHIFT:       return 0x36;

   // Numeric keypad
   case SDL_SCANCODE_KP_MULTIPLY:  return 0x37;
   case SDL_SCANCODE_KP_MINUS:     return 0x4A;
   case SDL_SCANCODE_KP_PLUS:      return 0x4E;
   case SDL_SCANCODE_KP_PERIOD:    return 0x53;
   case SDL_SCANCODE_KP_0:         return 0x52;
   case SDL_SCANCODE_KP_1:         return 0x4F;
   case SDL_SCANCODE_KP_2:         return 0x50;
   case SDL_SCANCODE_KP_3:         return 0x51;
   case SDL_SCANCODE_KP_4:         return 0x4B;
   case SDL_SCANCODE_KP_5:         return 0x4C;
   case SDL_SCANCODE_KP_6:         return 0x4D;
   case SDL_SCANCODE_KP_7:         return 0x47;
   case SDL_SCANCODE_KP_8:         return 0x48;
   case SDL_SCANCODE_KP_9:         return 0x49;

   // Extended keys
   case SDL_SCANCODE_KP_ENTER:     return 0x11C;
   case SDL_SCANCODE_RCTRL:        return 0x11D;
   case SDL_SCANCODE_KP_DIVIDE:    return 0x135;
   case SDL_SCANCODE_RALT:         return 0x138;
   case SDL_SCANCODE_HOME:         return 0x147;
   case SDL_SCANCODE_UP:           return 0x148;
   case SDL_SCANCODE_PAGEUP:       return 0x149;
   case SDL_SCANCODE_LEFT:         return 0x14B;
   case SDL_SCANCODE_RIGHT:        return 0x14D;
   case SDL_SCANCODE_END:          return 0x14F;
   case SDL_SCANCODE_DOWN:         return 0x150;
   case SDL_SCANCODE_PAGEDOWN:     return 0x151;
   case SDL_SCANCODE_INSERT:       return 0x152;
   case SDL_SCANCODE_DELETE:       return 0x153;
   case SDL_SCANCODE_LGUI:         return 0x15B;
   case SDL_SCANCODE_RGUI:         return 0x15C;
   case SDL_SCANCODE_APPLICATION:  return 0x15D;
   default:                        return 0;
   }
}

unsigned int SdlScancodeToHost(SDL_Scancode scancode)
{
   const unsigned int pc = ToPcScanCode(scancode);
#if defined(_WIN32)
   return pc;
#else
   // X11 keycodes (evdev) : the scan code + 8 for the base keys, their own values for the extended
   if (pc == 0) return 0;
   if (pc < 0x100) return pc + 8;
   switch (pc)
   {
   case 0x11C: return 104;   // Keypad enter
   case 0x11D: return 105;   // Right control
   case 0x135: return 106;   // Keypad divide
   case 0x138: return 108;   // Right alt
   case 0x147: return 110;   // Home
   case 0x148: return 111;   // Up
   case 0x149: return 112;   // Page up
   case 0x14B: return 113;   // Left
   case 0x14D: return 114;   // Right
   case 0x14F: return 115;   // End
   case 0x150: return 116;   // Down
   case 0x151: return 117;   // Page down
   case 0x152: return 118;   // Insert
   case 0x153: return 119;   // Delete
   case 0x15B: return 133;   // Left super
   case 0x15C: return 134;   // Right super
   case 0x15D: return 135;   // Menu
   default:    return 0;
   }
#endif
}
