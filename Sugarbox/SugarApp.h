#pragma once

#include <wx/wx.h>

#include "Motherboard.h"
#include "KeyboardHandler.h"
#include "Snapshot.h"

class SugarApp : public wxApp
{
public:
   SugarApp();
   virtual ~SugarApp();
   virtual bool OnInit();


protected:
   KeyboardHandler keyboard_handler_;
   CSnapshot sna_handler_;
   Motherboard* motherboard_;
};
