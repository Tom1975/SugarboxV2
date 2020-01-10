#pragma once

#include <wx/wx.h>

#include "Motherboard.h"
#include "KeyboardHandler.h"
#include "Screen.h"
#include "Emulation.h"

class SugarApp : public wxApp
{
public:
   SugarApp();
   virtual ~SugarApp();
   virtual bool OnInit();

protected:
   KeyboardHandler keyboard_handler_;
   IDisplay* display_;

   Emulation* emulation_engine_;

   std::thread* worker_thread_;
   std::atomic_bool finished_;
};
