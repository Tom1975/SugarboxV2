#include "Canvas.h"

static const int kCanvasMargin = 0;

Canvas::Canvas(
   wxWindow* parent,
   wxWindowID id,
   wxPoint position,
   wxSize size,
   long style ) :
   wxSfmlCanvas(parent, id, position, size, style)
{
}

void Canvas::Init(CDisplay* display)
{
   display_ = display;
}

void Canvas::onUpdate()
{
   display_->Display();
}

void Canvas::onResize(wxSizeEvent& event)
{
   auto size = event.GetSize();

   auto newCanvasWidth = size.x - (2 * kCanvasMargin);
   auto newCanvasHeight = size.y - (2 * kCanvasMargin);

   // Resize Canvas window
   setwxWindowSize({ newCanvasWidth, newCanvasHeight });
   setRenderWindowSize({ (unsigned int)newCanvasWidth, (unsigned int)newCanvasHeight });
}
