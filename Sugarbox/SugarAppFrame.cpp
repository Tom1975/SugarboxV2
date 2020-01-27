#include "SugarAppFrame.h"

static const int kDefaultWindowWidth = 1024;
static const int kDefaultWindowHeight = 500;
static const int kCanvasMargin = 0;

SugarAppFrame::SugarAppFrame(const wxString& title, const wxPoint& pos, const wxSize& size) :
      wxFrame(NULL, wxID_ANY, title, pos, size),
      _panel(new wxPanel(this)),
      _canvas(new Canvas(
         _panel.get(),
         wxID_ANY,
         wxPoint(kCanvasMargin, kCanvasMargin),
         wxSize(kDefaultWindowWidth - (2 * kCanvasMargin), kDefaultWindowHeight - (2 * kCanvasMargin))
      )),
      key_handler_(nullptr)
{
   display_.Init(_canvas.get());
   _canvas->Init(&display_);
   _panel->SetBackgroundColour(*wxCYAN);


   ////////////////////////////////////////////////////////////////////////////////
   // Probably due to some RTTI, IDE is getting confused by this dynamic call
   // and doesn't understand the correct Bind overload. Causes non sequitur errors
   // to display in the inspector. Suppress.
   //
   // Dynamic binding is cleanest here, since we don't want to hook up our resize
   // handler until our dependencies (Canvas namely) have finished their initialization
   ////////////////////////////////////////////////////////////////////////////////
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wint-conversion"
   Bind(wxEVT_SIZE, &SugarAppFrame::onResize, this);
   _canvas->Bind(wxEVT_KEY_DOWN, &SugarAppFrame::onKeyDown, this);
   _canvas->Bind(wxEVT_KEY_UP, &SugarAppFrame::onKeyUp, this);
#pragma clang diagnostic pop
   ////////////////////////////////////////////////////////////////////////////////

}

void SugarAppFrame::SetKeyboardHandler(IKeyboard* key_handler)
{
   key_handler_ = key_handler;
}

void SugarAppFrame::onKeyUp(wxKeyEvent& event)
{
   key_handler_->SendScanCode(event.GetKeyCode(), false);
   //pMainWindow->emulator_settings_.GetJoystickSettings()->joystick_1_.KeyDown((lParam >> 16) & 0x1FF);

   event.Skip();
}

void SugarAppFrame::onKeyDown(wxKeyEvent& event)
{
   key_handler_->SendScanCode( event.GetKeyCode(), true);
   //pMainWindow->emulator_settings_.GetJoystickSettings()->joystick_1_.KeyDown((lParam >> 16) & 0x1FF);

   event.Skip();
}

void SugarAppFrame ::onResize(wxSizeEvent& event)
{
   _canvas->onResize(event);
   event.Skip();
}
