#pragma once

#include "wxSFMLCanvas.h"
#include "Display.h"

class Canvas : public wxSfmlCanvas
{
public:
   Canvas(
      wxWindow* parent,
      wxWindowID id,
      wxPoint position,
      wxSize size,
      long style = 0);

   virtual void Init(CDisplay* display);
   virtual void onUpdate();
   void onResize(wxSizeEvent& event);

protected:
   CDisplay* display_;
};