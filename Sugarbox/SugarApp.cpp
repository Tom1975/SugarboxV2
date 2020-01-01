#include "SugarApp.h"
#include "SugarAppFrame.h"

static const int kDefaultWindowWidth = 1280;
static const int kDefaultWindowHeight = 720;

SugarApp::SugarApp() : wxApp  ()
{
   motherboard_ = new Motherboard (nullptr, nullptr);
}

SugarApp::~SugarApp()
{
   delete motherboard_;
}


bool SugarApp::OnInit()
{
   auto frame = new SugarAppFrame("Sugarbox", wxPoint(100, 100),
      wxSize(kDefaultWindowWidth, kDefaultWindowHeight));
   frame->Show(true);
   return true;
}
