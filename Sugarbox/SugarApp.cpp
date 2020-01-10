#include "SugarApp.h"
#include "SugarAppFrame.h"

#pragma once

static const int kDefaultWindowWidth = 1280;
static const int kDefaultWindowHeight = 720;

SugarApp::SugarApp() :  wxApp  (), 
   display_(nullptr),
   emulation_engine_(nullptr),
   worker_thread_(nullptr),
   finished_(false)
{
   emulation_engine_ = new Emulation();
}

SugarApp::~SugarApp()
{
}

void EmulationLoop(Emulation* emulation_engine)
{
   emulation_engine->EmulationLoop();
}


bool SugarApp::OnInit()
{
   // Create frame
   auto frame = new SugarAppFrame("Sugarbox", wxPoint(100, 100),
      wxSize(kDefaultWindowWidth, kDefaultWindowHeight));
   frame->Show(true);

   emulation_engine_->Init(&keyboard_handler_, frame->GetDisplay());
   
   // Create dedicated thread for emulation
   worker_thread_ = new std::thread( ::EmulationLoop, emulation_engine_);

   return true;
}
