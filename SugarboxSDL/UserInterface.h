#pragma once

#include <mutex>
#include <string>
#include <vector>

#include <SDL3/SDL.h>

#include "DiskBuilder.h"
#include "SettingsList.h"

#include "SDLDisplay.h"
#include "SettingsDialog.h"

class Emulation;
class SDLSound;

// What the user interface asks to the application
class IUserInterfaceHost : public ISettingsHost
{
public:
   virtual void LoadMedia(const std::string& path) = 0;
   virtual void ToggleFullScreen() = 0;
   virtual bool IsFullScreen() = 0;
   virtual void Quit() = 0;
};

// Menu bar, status bar and file dialogs of the SDL frontend, drawn with Dear ImGui over the
// emulated screen. Everything here runs on the main thread, but the file dialog callbacks.
class UserInterface : public IDisplayOverlay
{
public:
   struct Statistics
   {
      unsigned int speed = 0;
      double emulated_frames = 0.0;
      double shown_frames = 0.0;
   };

   bool Init(SDL_Window* window, SDLDisplay* display, Emulation* emulation, SDLSound* sound, IUserInterfaceHost* host, float scale);
   void Shutdown();

   bool IsReady() const { return initialised_; }

   void ProcessEvent(const SDL_Event& event);
   // A text is edited in the interface : the keys are not for the emulated keyboard
   bool WantsKeyboard() const;

   // Builds the interface of the next frame shown
   void BuildFrame(const Statistics& statistics);
   // Height (pixels) taken at the top and at the bottom of the window : the emulated screen is
   // shown between them
   int GetTopMargin() const { return top_margin_; }
   int GetBottomMargin() const { return bottom_margin_; }

   // Files chosen in the dialogs since the last call
   void HandleDialogResults();

   // Screenshot in the pictures folder (Pictures/Sugarbox), named after the date
   void QuickScreenshot();

   // The media in the drive / the tape player is about to be replaced : false to keep it
   bool ConfirmDiskChange(int drive);
   bool ConfirmTapeChange();

   // IDisplayOverlay
   void Prepare(SDL_GPUCommandBuffer* command_buffer) override;
   void Draw(SDL_GPUCommandBuffer* command_buffer, SDL_GPURenderPass* render_pass) override;

protected:
   enum DialogKind
   {
      DLG_OPEN_MEDIA,
      DLG_RUN_SCRIPT,
      DLG_INSERT_DISK,
      DLG_SAVE_DISK,
      DLG_INSERT_TAPE,
      DLG_SAVE_TAPE,
      DLG_LOAD_SNA,
      DLG_SAVE_SNA,
      DLG_LOAD_SNR,
      DLG_RECORD_SNR,
      DLG_LOAD_CPR,
      DLG_SAVE_SCREENSHOT,
   };
   struct DialogRequest
   {
      UserInterface* ui;
      DialogKind kind;
      int drive;
   };
   struct DialogResult
   {
      DialogKind kind;
      int drive;
      std::string path;
      int filter;
   };

   void OpenDialog(DialogKind kind, int drive = 0);
   static void SDLCALL DialogCallback(void* userdata, const char* const* filelist, int filter);
   void HandleDialogResult(const DialogResult& result);
   bool Confirm(const char* message);
   void Screenshot(const std::string& path);
   // Shown for a few seconds in the status bar
   void ShowMessage(const std::string& message);

   void DrawMenuBar();
   void DrawDiskMenu(int drive);
   void DrawStatusBar(const Statistics& statistics);
   void DrawDriveStatus(int drive);
   bool IsSpeed(int limit, int speed) const;
   void SetSpeed(int limit, int speed);

   SDL_Window* window_ = nullptr;
   SDLDisplay* display_ = nullptr;
   Emulation* emulation_ = nullptr;
   SDLSound* sound_ = nullptr;
   IUserInterfaceHost* host_ = nullptr;
   bool initialised_ = false;

   int top_margin_ = 0;
   int bottom_margin_ = 0;
   bool popup_open_ = false;
   std::string message_;
   Uint64 message_end_ = 0;

   SettingsDialog settings_dialog_;

   // File dialogs : the filters must live until the dialog is closed
   DiskBuilder disk_builder_;
   std::vector<FormatType*> disk_save_formats_;
   std::vector<std::string> filter_strings_;
   std::vector<SDL_DialogFileFilter> disk_load_filters_;
   std::vector<SDL_DialogFileFilter> disk_save_filters_;
   std::string media_pattern_;
   std::vector<SDL_DialogFileFilter> media_filters_;

   std::mutex results_mutex_;
   std::vector<DialogResult> results_;
};
