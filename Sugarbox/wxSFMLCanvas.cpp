#include "wxSFMLCanvas.h"

wxBEGIN_EVENT_TABLE(wxSfmlCanvas, wxControl)
EVT_IDLE(wxSfmlCanvas::onIdle)
EVT_PAINT(wxSfmlCanvas::onPaint)
EVT_ERASE_BACKGROUND(wxSfmlCanvas::onEraseBackground)
wxEND_EVENT_TABLE()

wxSfmlCanvas::wxSfmlCanvas(
      wxWindow* parent,
      wxWindowID windowId,
      const wxPoint& position,
      const wxSize& size,
      long style) :
      wxControl(parent, windowId, position, size, style) 
{
   createRenderWindow();
}

wxSfmlCanvas::~wxSfmlCanvas()
{
}

void wxSfmlCanvas::onUpdate()
{
}

void wxSfmlCanvas::onIdle(wxIdleEvent& event)
{
   // Send a paint message when control is idle, to ensure max framerate
   Refresh();
}

void wxSfmlCanvas::onPaint(wxPaintEvent& event)
{
   wxPaintDC dc(this);     // Prepare control to be repainted
   onUpdate();             // Tick update
   display();              // Draw
}

// Explicitly overriding prevents wxWidgets from drawing, which could result in flicker
void wxSfmlCanvas::onEraseBackground(wxEraseEvent& event) 
{
}

void wxSfmlCanvas::createRenderWindow()
{
#ifdef __WXGTK__
   gtk_widget_realize(m_wxwindow);
   gtk_widget_set_double_buffered(m_wxwindow, false);

   GdkWindow* gdkWindow = gtk_widget_get_window((GtkWidget*)GetHandle());
   XFlush(GDK_WINDOW_XDISPLAY(gdkWindow));

   sf::RenderWindow::create(GDK_WINDOW_XWINDOW(gdkWindow));
#else
   sf::RenderWindow::create(GetHandle());
#endif
}

void wxSfmlCanvas::setwxWindowSize(const wxSize& size)
{
   SetSize(size);
}

void wxSfmlCanvas::setRenderWindowSize(const sf::Vector2u& size)
{
   setSize(size);
}

