#pragma once

#include <functional>
#include <map>

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

class SugarboxApp : public ISoundFactory, public IFunctionInterface
{
public:
   SugarboxApp();
   virtual ~SugarboxApp();

   int RunApp();

   // IFunctionInterface interface
   virtual void Exit();

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

   // Display gui
   void RunMainLoop();
   void DrawMainWindow();
   void DrawMenu();
   void DrawPeripherals();
   void DrawStatusBar();
   void HandlePopups();

   void AskForSaving(int drive);

   // Gui related
   enum {
      POPUP_NONE,
      POPUP_ASK_SAVE
   } PopupType;
   unsigned int PopupArg;

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
};

