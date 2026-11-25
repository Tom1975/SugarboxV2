#include "AppSettings.h"

#include <cstdlib>
#include <filesystem>

#include "ConfigurationManager.h"

namespace
{
   const char* kCurrent = "Current";
   const char* kSection = "SugarboxSDL";
   const char* kKeyNames[AppSettings::KEY_COUNT] = { "Up", "Down", "Left", "Right", "Fire1", "Fire2" };

   class IniReader
   {
   public:
      IniReader(ConfigurationManager& manager) : manager_(manager) {}

      std::string String(const char* section, const char* key, const std::string& default_value)
      {
         char buffer[1024];
         manager_.GetConfiguration(section, key, default_value.c_str(), buffer, sizeof(buffer));
         return buffer;
      }
      int Int(const char* section, const char* key, int default_value)
      {
         const std::string value = String(section, key, "");
         return value.empty() ? default_value : std::atoi(value.c_str());
      }
      bool Bool(const char* section, const char* key, bool default_value)
      {
         return Int(section, key, default_value ? 1 : 0) != 0;
      }

   private:
      ConfigurationManager& manager_;
   };
}

AppSettings::AppSettings()
{
   // Keyboard joystick : the arrows, Space and Left Alt
   for (auto& joystick : joysticks)
   {
      joystick.keys[KEY_UP] = SDL_SCANCODE_UP;
      joystick.keys[KEY_DOWN] = SDL_SCANCODE_DOWN;
      joystick.keys[KEY_LEFT] = SDL_SCANCODE_LEFT;
      joystick.keys[KEY_RIGHT] = SDL_SCANCODE_RIGHT;
      joystick.keys[KEY_FIRE1] = SDL_SCANCODE_SPACE;
      joystick.keys[KEY_FIRE2] = SDL_SCANCODE_LALT;
   }
}

void AppSettings::Load(const char* ini_file)
{
   ConfigurationManager manager;
   manager.OpenFile(ini_file);
   IniReader ini(manager);

   startup_machine = ini.String(kCurrent, "Machine_Settings", startup_machine);
   // Not the LimitSpeed of [Current] : the Qt application and the installed file use full speed
   const std::string limit = ini.String(kSection, "LimitSpeed", std::string(1, limit_speed));
   limit_speed = limit.empty() ? 'G' : limit[0];
   speed = ini.Int(kSection, "Speed", speed);
   reload_disks = ini.Bool(kSection, "ReloadDisks", reload_disks);
   disks[0] = ini.String(kCurrent, "FD1_Path", "");
   disks[1] = ini.String(kCurrent, "FD2_Path", "");

   volume = ini.Int(kSection, "Volume", static_cast<int>(volume * 100.0f + 0.5f)) / 100.0f;
   mute = ini.Bool(kSection, "Mute", mute);
   autoload = ini.Bool(kSection, "Autoload", autoload);
   screenshot_dir = ini.String(kSection, "ScreenshotDir", screenshot_dir);

   scaling = static_cast<Scaling>(SDL_clamp(ini.Int(kSection, "Scaling", scaling), SCALE_FIT, SCALE_STRETCH));
   smooth = ini.Bool(kSection, "Smooth", smooth);
   scanlines = ini.Bool(kSection, "Scanlines", scanlines);
   scanline_brightness = SDL_clamp(ini.Int(kSection, "ScanlineBrightness", scanline_brightness), 0, 100);
   start_full_screen = ini.Bool(kSection, "FullScreen", start_full_screen);
   show_status_bar = ini.Bool(kSection, "StatusBar", show_status_bar);
   window_x = ini.Int(kSection, "WindowX", window_x);
   window_y = ini.Int(kSection, "WindowY", window_y);
   window_width = ini.Int(kSection, "WindowWidth", window_width);
   window_height = ini.Int(kSection, "WindowHeight", window_height);

   for (int index = 0; index < 2; index++)
   {
      Joystick& joystick = joysticks[index];
      const std::string prefix = "Joystick" + std::to_string(index + 1);
      joystick.source = static_cast<JoystickSource>(SDL_clamp(ini.Int(kSection, (prefix + "Source").c_str(), joystick.source), JOY_NONE, JOY_KEYBOARD));
      joystick.gamepad = ini.String(kSection, (prefix + "Gamepad").c_str(), joystick.gamepad);
      for (int key = 0; key < KEY_COUNT; key++)
      {
         const std::string name = ini.String(kSection, (prefix + kKeyNames[key]).c_str(), "");
         const SDL_Scancode code = name.empty() ? SDL_SCANCODE_UNKNOWN : SDL_GetScancodeFromName(name.c_str());
         if (code != SDL_SCANCODE_UNKNOWN) joystick.keys[key] = code;
      }
   }
}

bool AppSettings::Save(const char* ini_file) const
{
   // The whole file is read, the keys are changed, and the file is written again
   ConfigurationManager manager;
   manager.OpenFile(ini_file);
   auto set = [&](const char* section, const std::string& key, const std::string& value) {
      manager.SetConfiguration(section, key.c_str(), value.c_str());
   };
   auto set_int = [&](const char* section, const std::string& key, int value) { set(section, key, std::to_string(value)); };

   set(kCurrent, "Machine_Settings", startup_machine);
   set(kSection, "LimitSpeed", std::string(1, limit_speed));
   set_int(kSection, "Speed", speed);
   set_int(kSection, "ReloadDisks", reload_disks);
   set(kCurrent, "FD1_Path", reload_disks ? disks[0] : std::string());
   set(kCurrent, "FD2_Path", reload_disks ? disks[1] : std::string());

   set_int(kSection, "Volume", static_cast<int>(volume * 100.0f + 0.5f));
   set_int(kSection, "Mute", mute);
   set_int(kSection, "Autoload", autoload);
   set(kSection, "ScreenshotDir", screenshot_dir);

   set_int(kSection, "Scaling", scaling);
   set_int(kSection, "Smooth", smooth);
   set_int(kSection, "Scanlines", scanlines);
   set_int(kSection, "ScanlineBrightness", scanline_brightness);
   set_int(kSection, "FullScreen", start_full_screen);
   set_int(kSection, "StatusBar", show_status_bar);
   set_int(kSection, "WindowX", window_x);
   set_int(kSection, "WindowY", window_y);
   set_int(kSection, "WindowWidth", window_width);
   set_int(kSection, "WindowHeight", window_height);

   for (int index = 0; index < 2; index++)
   {
      const Joystick& joystick = joysticks[index];
      const std::string prefix = "Joystick" + std::to_string(index + 1);
      set_int(kSection, prefix + "Source", joystick.source);
      set(kSection, prefix + "Gamepad", joystick.gamepad);
      for (int key = 0; key < KEY_COUNT; key++)
      {
         set(kSection, prefix + kKeyNames[key], SDL_GetScancodeName(joystick.keys[key]));
      }
   }

   manager.CloseFile();
   return true;
}

std::string AppSettings::GetScreenshotFolder() const
{
   if (!screenshot_dir.empty()) return screenshot_dir;

   const char* pictures = SDL_GetUserFolder(SDL_FOLDER_PICTURES);
   const std::filesystem::path folder = (pictures != nullptr) ? std::filesystem::u8path(pictures) : std::filesystem::current_path();
   return (folder / "Sugarbox").u8string();
}
