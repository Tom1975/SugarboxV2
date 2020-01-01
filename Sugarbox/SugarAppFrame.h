#pragma once

#include <memory>
#include <wx/wx.h>
#include "Canvas.h"

class SugarAppFrame : public wxFrame
{
public:
   SugarAppFrame(const wxString& title, const wxPoint& pos, const wxSize& size);
   void onResize(wxSizeEvent& event);

protected:
   std::unique_ptr<wxPanel> _panel;
   std::unique_ptr<Canvas> _canvas;
};
