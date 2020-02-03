#pragma once

#include "Emulation.h"
#include "Display.h"
#include "SFML/Graphics.hpp"

#include "Machine.h"
#include "Motherboard.h"
#include "Snapshot.h"
#include "ConfigurationManager.h"

class SugarboxApp
{
public:
   SugarboxApp();
   virtual ~SugarboxApp();

   int RunApp();
   
protected:
   // 
   void DrawMainWindow();
   void DrawMenu();
   void DrawPeripherals();
   void DrawStatusBar();

   // Screen position
   const float main_display_width = 800;
   const float main_display_height = 600;
   const float toolbar_height = 50;
   const float status_height = 50;
   const float peripherals_width = 100;

   float window_width_;
   float window_height_;

   Emulation emulation;
   CDisplay display;

   // counters
   char str_speed_[16];
   int counter_;
};