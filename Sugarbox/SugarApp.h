#pragma once

#include <wx/wx.h>

#include "Motherboard.h"

class SugarApp : public wxApp
{
public:
   SugarApp();
   virtual ~SugarApp();
   virtual bool OnInit();


protected:
   Motherboard* motherboard_;
};
