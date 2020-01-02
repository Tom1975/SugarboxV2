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


protected:
   KeyboardHandler keyboard_handler_;
   CSnapshot sna_handler_;

   ConfigurationManager config_manager_;
   IDisplay* display_;
   Motherboard* motherboard_;
};
