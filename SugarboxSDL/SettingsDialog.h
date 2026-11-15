#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <SDL3/SDL.h>

#include "AppSettings.h"

class SettingsList;

// What the dialog needs from the application
class ISettingsHost
{
public:
   // Preferences in use
   virtual AppSettings& GetSettings() = 0;
   // The preferences changed : applied to the running emulator, and saved
   virtual void ApplySettings() = 0;
   // Machines of the CONF directory
   virtual SettingsList* GetMachines() = 0;
};

// Settings window : one tab per part. It works on a copy of the preferences : Apply and OK
// hand it over, Cancel drops it.
class SettingsDialog
{
public:
   void Init(SDL_Window* window, ISettingsHost* host);

   void Open();
   bool IsOpen() const { return open_; }
   // Main thread, in the ImGui frame
   void Draw();

protected:
   void DrawEmulatorTab();
   void Apply();

   // Folder dialog : the result comes from another thread
   static void SDLCALL FolderCallback(void* userdata, const char* const* filelist, int filter);

   SDL_Window* window_ = nullptr;
   ISettingsHost* host_ = nullptr;
   bool open_ = false;
   bool just_opened_ = false;

   AppSettings edited_;

   std::mutex folder_mutex_;
   std::string chosen_folder_;
   bool folder_chosen_ = false;
};
