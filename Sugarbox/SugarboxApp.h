#pragma once

#include "Emulation.h"
#include "Display.h"
#include "SFMLSoundMixer.h"
#include "SFML/Graphics.hpp"

#include "ISound.h"
#include "Machine.h"
#include "Motherboard.h"
#include "Snapshot.h"
#include "ConfigurationManager.h"

class SugarboxApp : public ISoundFactory
{
public:
   SugarboxApp();
   virtual ~SugarboxApp();

   int RunApp();


   // ISoundFactory interface
   virtual ISound* GetSound(const char* name);
   virtual const char* GetSoundName(ISound*);

   virtual const char* GetFirstSoundName();
   virtual const char* GetNextSoundName();
   
protected:
   // 
   void RunMainLoop();
   void DrawMainWindow();
   void DrawMenu();
   void DrawPeripherals();
   void DrawStatusBar();

   // Display gui
   sf::RenderWindow* window_;
   IKeyboard* keyboard_handler_;

   // Screen position
   const float main_display_width = 800;
   const float main_display_height = 600;
   const float toolbar_height = 50;
   const float status_height = 50;
   const float peripherals_width = 100;

   float window_width_;
   float window_height_;

   Emulation emulation_;
   CDisplay display_;
   SFMLSoundMixer sound_mixer_;

   // counters
   char str_speed_[16];
   int counter_;
};