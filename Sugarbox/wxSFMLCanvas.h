#pragma once

#include <wx/wx.h>
#include <SFML/Graphics.hpp>

class wxSfmlCanvas : public wxControl, public sf::RenderWindow
{
public:
   wxSfmlCanvas(
      wxWindow* parent = nullptr,
      wxWindowID windowId = -1,
      const wxPoint& position = wxDefaultPosition,
      const wxSize& size = wxDefaultSize,
      long style = 0);
   virtual ~wxSfmlCanvas();

   virtual void onUpdate();
   void onIdle(wxIdleEvent& event);
   void onPaint(wxPaintEvent& event);
   void onEraseBackground(wxEraseEvent& event);

   void createRenderWindow();
   void setwxWindowSize(const wxSize& size);
   void setRenderWindowSize(const sf::Vector2u& size);
   
   wxDECLARE_EVENT_TABLE();
};
