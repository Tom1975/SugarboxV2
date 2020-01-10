#include "Emulation.h"

Emulation::Emulation() :
   motherboard_(nullptr), 
   sna_handler_(nullptr)
{
}

Emulation::~Emulation()
{
   delete motherboard_;
}

void Emulation::Init(KeyboardHandler* keyboard_handler, IDisplay* display)
{
   motherboard_ = new Motherboard(nullptr, keyboard_handler);

   // Create emulation
   motherboard_->SetPlus(true);
   motherboard_->InitMotherbard(nullptr, &sna_handler_, display, nullptr, nullptr, &config_manager_);
   motherboard_->OnOff();
   motherboard_->GetMem()->InitMemory();
   motherboard_->GetMem()->SetRam(1);
   motherboard_->GetCRTC()->DefinirTypeCRTC(CRTC::AMS40226);
   motherboard_->GetVGA()->SetPAL(true);
   motherboard_->GetPSG()->Reset();
   motherboard_->GetSig()->Reset();
   motherboard_->InitStartOptimizedPlus();
   motherboard_->OnOff();


}

void Emulation::EmulationLoop()
{
   while (true)
   {
      motherboard_->StartOptimizedPlus<true, true, true>(40000);

   }
}