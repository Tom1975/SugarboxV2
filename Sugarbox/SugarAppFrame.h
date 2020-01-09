#pragma once

#include <memory>
#include <wx/wx.h>
#include "Canvas.h"
#include "Display.h"

class SugarAppFrame : public wxFrame
{
public:
   SugarAppFrame(const wxString& title, const wxPoint& pos, const wxSize& size);
   void onResize(wxSizeEvent& event);

   CDisplay* GetDisplay() 
   {      
      return &display_;   
   }

protected:
   std::unique_ptr<wxPanel> _panel;
   std::unique_ptr<Canvas> _canvas;
   CDisplay display_;
};
