#include "Emulation.h"

Emulation::Emulation() :
   motherboard_(nullptr), 
   sna_handler_(nullptr),
   running_thread_(false),
   worker_thread_(nullptr)
{
}

Emulation::~Emulation()
{
   delete motherboard_;
}

IKeyboard* Emulation::GetKeyboardHandler()
{
   return emulator_engine_->GetKeyboardHandler();
}

void RunLoop(Emulation* emulator)
{
   emulator->EmulationLoop();
}

void Emulation::Init( IDisplay* display)
{
   emulator_settings_.Init(&config_manager_, nullptr);
   emulator_settings_.Load("Sugarbox.ini");

   emulator_engine_ = new EmulatorEngine();
   emulator_engine_->SetConfigurationManager(&config_manager_);
   emulator_engine_->Init(display, nullptr);

   emulator_engine_->SetDefaultConfiguration();
   emulator_engine_->SetSettings(emulator_settings_);

   emulator_engine_->LoadConfiguration("Current", "Sugarbox.ini");

   emulator_engine_->GetMem()->Initialisation();
   // Update computer
   emulator_engine_->Reinit();

   running_thread_ = true;
   worker_thread_ = new std::thread(RunLoop, this);
}

void Emulation::Stop()
{
   running_thread_ = false;
   worker_thread_->join();
   delete worker_thread_;
}

void Emulation::EmulationLoop()
{
   while (running_thread_)
   {
      emulator_engine_->RunTimeSlice(true);
   }
}