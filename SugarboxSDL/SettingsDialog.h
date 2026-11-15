#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <SDL3/SDL.h>

#include "AppSettings.h"

class MachineSettings;
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
   // Reads the CONF directory again (a machine was added, changed or removed)
   virtual void ReloadMachines() = 0;
   // The machine the emulation runs
   virtual bool IsCurrentMachine(const MachineSettings& machine) = 0;
   // Runs this machine (power on again)
   virtual void ChangeMachine(const std::string& path) = 0;
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
   void DrawMachineTab();
   void DrawMachineDetail();
   void Apply();

   // Machine tab
   void ScanFiles();
   void SelectMachine(const std::string& path);
   // false : the changes of the machine edited are kept (the user cancelled)
   bool ConfirmDiscardMachine();
   bool SaveMachine();
   void DuplicateMachine(const std::string& name);
   void DeleteMachine();
   bool Ask(const char* message, const char* yes, const char* no);

   // File and folder dialogs : the result comes from another thread
   enum PathKind { PATH_SCREENSHOT_FOLDER, PATH_CARTRIDGE };
   void ShowPathDialog(PathKind kind, const char* default_location);
   static void SDLCALL PathCallback(void* userdata, const char* const* filelist, int filter);
   void HandlePathResults();

   SDL_Window* window_ = nullptr;
   ISettingsHost* host_ = nullptr;
   bool open_ = false;
   bool just_opened_ = false;

   AppSettings edited_;

   // Machine tab : a copy of the machine selected, saved with its Save button (or Apply / OK)
   std::unique_ptr<MachineSettings> machine_;
   bool machine_dirty_ = false;
   bool show_all_roms_ = false;
   std::vector<std::string> roms_;
   std::vector<std::string> keyboards_;
   char new_name_[128] = {};
   std::string new_name_error_;

   struct PathRequest
   {
      SettingsDialog* dialog;
      PathKind kind;
   };
   std::mutex path_mutex_;
   std::vector<std::pair<PathKind, std::string>> path_results_;
};
