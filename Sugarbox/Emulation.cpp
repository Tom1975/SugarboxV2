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
   emulator_engine_ = new EmulatorEngine();

   emulator_engine_->SetConfigurationManager(&config_manager_);
   emulator_engine_->SetDefaultConfiguration();
   emulator_engine_->SetSettings(emulator_settings_);
   emulator_engine_->Init(display, nullptr);
   emulator_engine_->GetMem()->Initialisation();
   // Update computer
   emulator_engine_->Reinit();

}

void Emulation::EmulationLoop()
{
   while (true)
   {
      emulator_engine_->RunTimeSlice(true);

   }
}