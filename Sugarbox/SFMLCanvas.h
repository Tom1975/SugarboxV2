#include <SFML/Graphics.hpp>
#include <wx/wx.h>

class wxSFMLCanvas : public wxControl, public sf::RenderWindow
{
public:

   wxSFMLCanvas(wxWindow* Parent = NULL, wxWindowID Id = -1, const wxPoint& Position = wxDefaultPosition,
      const wxSize& Size = wxDefaultSize, long Style = 0);

   virtual ~wxSFMLCanvas() {};

private:

   DECLARE_EVENT_TABLE()

   virtual void OnUpdate() {};

   void OnIdle(wxIdleEvent&);

   void OnPaint(wxPaintEvent&);

   void OnEraseBackground(wxEraseEvent&) {};
};