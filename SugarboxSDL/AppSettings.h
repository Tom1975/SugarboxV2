#pragma once

#include <string>

#include <SDL3/SDL.h>

// Preferences of the SDL frontend, in Sugarbox.ini :
// - [Current] : the keys the core reads at startup (machine, disks), shared with the Qt
//   application
// - [SugarboxSDL] : everything else
class AppSettings
{
public:
   // Startup
   std::string startup_machine = "CONF/CPC6128PLUSEN.cfg";
   // LimitSpeed : F (full), V (VSync), S (sound), G (G-Sync), C (custom, with speed)
   char limit_speed = 'G';
   int speed = 100;
   // The disks in the drives at exit are inserted again at the next start
   bool reload_disks = false;
   // FD1_Path, FD2_Path : inserted by the core at startup
   std::string disks[2];

   // Emulator
   float volume = 1.0f;
   bool mute = false;
   bool autoload = true;
   // Empty : Pictures/Sugarbox
   std::string screenshot_dir;

   // Display
   enum Scaling { SCALE_FIT, SCALE_INTEGER, SCALE_STRETCH };
   Scaling scaling = SCALE_FIT;
   bool smooth = false;
   bool scanlines = false;
   // Brightness of the darkened lines, in percent
   int scanline_brightness = 60;
   bool start_full_screen = false;
   bool show_status_bar = true;
   // Window at exit (-1 : not known yet)
   int window_x = -1;
   int window_y = -1;
   int window_width = -1;
   int window_height = -1;

   // Input : the two joysticks of the CPC
   enum JoystickSource { JOY_NONE, JOY_GAMEPAD, JOY_KEYBOARD };
   enum JoystickKey { KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_FIRE1, KEY_FIRE2, KEY_COUNT };
   struct Joystick
   {
      JoystickSource source = JOY_NONE;
      // Gamepad, found by its name (empty : the first one not used by the other joystick)
      std::string gamepad;
      SDL_Scancode keys[KEY_COUNT] = {};
   };
   Joystick joysticks[2];

   AppSettings();

   // Missing keys keep their default value
   void Load(const char* ini_file);
   bool Save(const char* ini_file) const;

   // Folder of the screenshots taken with F12
   std::string GetScreenshotFolder() const;
};
