#pragma once

#include <functional>
#include <map>

#include <imgui.h>


#include "Emulation.h"
#include "Display.h"
#include "ALSoundMixer.h"

#include "ISound.h"
#include "Machine.h"
#include "Motherboard.h"
#include "Snapshot.h"
#include "ConfigurationManager.h"
#include "MultiLanguage.h"
#include "Functions.h"
#include "SettingsList.h"
#include "DlgSettings.h"
#include "SoundControl.h"

class SugarboxApp : public ISoundFactory, public IFunctionInterface
{
public:
   SugarboxApp();
   virtual ~SugarboxApp();

   int RunApp();

   // IFunctionInterface interface
   virtual void Exit();
   virtual void HardReset();
   virtual void Pause();
   virtual bool PauseEnabled();
   virtual void SetSpeed(int speedlimit);
   virtual void ConfigurationSettings();
   virtual void SaveAs(int drive);
   virtual void Eject(int drive);
   virtual bool DiskPresent(int drive);
   virtual void Flip(int drive);
   virtual void Insert(int drive);
   virtual void InsertBlank(int drive, IDisk::DiskType type);
   virtual void TapeRecord();
   virtual void TapePlay();
   virtual void TapeFastForward();
   virtual void TapeRewind();
   virtual void TapePause();
   virtual void TapeStop();
   virtual void TapeInsert();
   virtual void TapeSaveAs(Emulation::TapeFormat format);
   virtual bool IsQuickSnapAvailable();
   virtual void SnaLoad();
   virtual void SnaSave();
   virtual void SnaQuickLoad();
   virtual void SnaQuickSave();
   virtual void SnrLoad();
   virtual void SnrRecord();
   virtual void SnrStopRecord();
   virtual void SnrStopPlayback();
   virtual bool SnrIsRecording();
   virtual bool SnrIsReplaying();

   // ISoundFactory interface
   virtual ISound* GetSound(const char* name);
   virtual const char* GetSoundName(ISound*);

   virtual const char* GetFirstSoundName();
   virtual const char* GetNextSoundName();
   
   // Events
   void SizeChanged(int width, int height);
   void Drop( int count, const char** paths);
   void KeyboardHandler( int key, int scancode, int action, int mods);

protected:
   // Menu init
   void InitMenu();
   void InitFileDialogs();

   // Display gui
   void RunMainLoop();
   void DrawMainWindow();
   void DrawMenu();
   void DrawSubMenu(Function*);
   void DrawPeripherals();
   void DrawStatusBar();
   void DrawOthers();
   void HandlePopups();

   bool AskForSaving(int drive);
   void InsertSelectFile(int drive);
   void InsertBlankDisk(int drive, IDisk::DiskType type);
   bool AskForSavingTape();
   void InsertSelectTape();

   // Gui related
   enum {
      POPUP_NONE,
      POPUP_ASK_SAVE
   } PopupType;
   unsigned int PopupArg;
   std::function<void()> popup_associated_function_;
   

   //////////////
   // File dialogs
   enum {
      FD_SAVE_AS,
      FD_INSERT,
      FD_INSERT_TAPE,
      FD_SAVE_TAPE_AS,
      FD_INSERT_SNA,
      FD_SAVE_SNA,
      FD_LOAD_SNR,
      FD_RECORD_SNR
   } file_dialog_type_;

   std::map<std::string, const FormatType*> format_ext_map_;
   std::map<std::string, const FormatType*> format_ext_map_read_;
   char* write_disk_extension_;
   char* load_disk_extension_;
   char* load_tape_extension_;
   Emulation::TapeFormat format_;

   // Keyboard handler
   IKeyboard* keyboard_handler_;

   // Screen position
   const float main_display_width = 768;
   const float main_display_height = 544;
   const float toolbar_height = 50;
   const float status_height = 50;
   const float peripherals_width = 100;

   float window_width_;
   float window_height_;

   // Emulation and so on 
   ConfigurationManager config_manager_;
   Emulation emulation_;
   CDisplay display_;
   ALSoundMixer sound_mixer_;
   GLFWwindow* window_;
   // counters
   char str_speed_[16];
   int counter_;

   // Functions
   MultiLanguage language_;
   FunctionList functions_list_;

   // Opened / close windows
   DiskBuilder disk_builder_;
   DlgSettings dlg_settings_;
   SoundControl sound_control_;
   bool configuration_settings_;
};

