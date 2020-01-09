#pragma once

#include <wx/wx.h>

#include "Motherboard.h"
#include "KeyboardHandler.h"
#include "Snapshot.h"
#include "Screen.h"
#include "ConfigurationManager.h"

class SugarApp : public wxApp
{
public:
   SugarApp();
   virtual ~SugarApp();
   virtual bool OnInit();

   void EmulationLoop();

protected:
   KeyboardHandler keyboard_handler_;
   CSnapshot sna_handler_;

   ConfigurationManager config_manager_;
   IDisplay* display_;
   Motherboard* motherboard_;

   std::thread* worker_thread_;
   std::atomic_bool finished_;
};
