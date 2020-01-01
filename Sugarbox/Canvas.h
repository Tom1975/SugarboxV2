#pragma once

#include "wxSfmlCanvas.h"

class Canvas : public wxSfmlCanvas
{
public:
   Canvas(
      wxWindow* parent,
      wxWindowID id,
      wxPoint position,
      wxSize size,
      long style = 0);

   virtual void onUpdate();
   void onResize(wxSizeEvent& event);
};