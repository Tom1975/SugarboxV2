#include "SFMLCanvas.h"

#ifdef __WXGTK__
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <wx/gtk/win_gtk.h>
#endif

wxSFMLCanvas::wxSFMLCanvas(wxWindow* Parent, wxWindowID Id, const wxPoint& Position, const wxSize& Size, long Style) :
   wxControl(Parent, Id, Position, Size, Style), sf::RenderWindow(GetHandle())
{
#ifdef __WXGTK__

   // GTK implementation requires to go deeper to find the
   // low-level X11 identifier of the widget
   gtk_widget_realize(m_wxwindow);
   gtk_widget_set_double_buffered(m_wxwindow, false);
   GdkWindow* Win = GTK_PIZZA(m_wxwindow)->bin_window;
   XFlush(GDK_WINDOW_XDISPLAY(Win));
   sf::RenderWindow::Create(GDK_WINDOW_XWINDOW(Win));

#else

   // Tested under Windows XP only (should work with X11
   // and other Windows versions - no idea about MacOS)
   //sf::RenderWindow::Create(GetHandle());

#endif
}

void wxSFMLCanvas::OnIdle(wxIdleEvent&)
{
   // Send a paint message when the control is idle, to ensure maximum framerate
   Refresh();
}

void wxSFMLCanvas::OnPaint(wxPaintEvent&)
{
   // Prepare the control to be repainted
   wxPaintDC Dc(this);

   // Let the derived class do its specific stuff
   OnUpdate();

   // Display on screen
   //Display();
}

