#pragma once

#include <memory>
#include <wx/wx.h>
#include "Canvas.h"
#include "Display.h"
#include "IKeyboard.h"

class SugarAppFrame : public wxFrame
{
public:
   SugarAppFrame(const wxString& title, const wxPoint& pos, const wxSize& size);

   void SetKeyboardHandler(IKeyboard* key_handler);

   CDisplay* GetDisplay()
   {
      return &display_;
   }

   // Event Handler 
   void onResize(wxSizeEvent& event);
   void onKeyDown(wxKeyEvent& event);
   void onKeyUp(wxKeyEvent& event);


protected:
   std::unique_ptr<wxPanel> _panel;
   std::unique_ptr<Canvas> _canvas;
   CDisplay display_;

   IKeyboard* key_handler_;
};
