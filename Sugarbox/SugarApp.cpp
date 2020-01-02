#include "SugarApp.h"
#include "SugarAppFrame.h"

static const int kDefaultWindowWidth = 1280;
static const int kDefaultWindowHeight = 720;

SugarApp::SugarApp() :  wxApp  (), 
                        sna_handler_(nullptr)
{

   motherboard_ = new Motherboard (nullptr, &keyboard_handler_);
}

SugarApp::~SugarApp()
{
   delete motherboard_;
}


bool SugarApp::OnInit()
{
   // Create frame
   auto frame = new SugarAppFrame("Sugarbox", wxPoint(100, 100),
      wxSize(kDefaultWindowWidth, kDefaultWindowHeight));
   frame->Show(true);

   // Create emulation
   motherboard_->InitMotherbard(nullptr, &sna_handler_, );

   return true;
}
