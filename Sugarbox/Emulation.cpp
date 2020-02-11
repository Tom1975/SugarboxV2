#include "Emulation.h"

//////////////////////////////////////////////
/// ctor/dtor
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


//////////////////////////////////////////////
///
IKeyboard* Emulation::GetKeyboardHandler()
{
   return emulator_engine_->GetKeyboardHandler();
}

void RunLoop(Emulation* emulator)
{
   emulator->EmulationLoop();
}

unsigned int Emulation::GetSpeed()
{
   return emulator_engine_->GetSpeed();
}

void Emulation::Init( IDisplay* display, ISoundFactory* sound)
{
   emulator_settings_.Init(&config_manager_, sound);
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
   emulator_engine_->Stop();

}

DataContainer* Emulation::CanLoad(const char* file, std::vector<MediaManager::MediaType>list_of_types)
{
   return emulator_engine_->CanLoad(file, list_of_types);
}

bool Emulation::LoadSnr(const char* path_file)
{
   return emulator_engine_->LoadSnr(path_file);
}

bool Emulation::LoadBin(const char* path_file)
{
   return emulator_engine_->LoadBin(path_file);
}

bool Emulation::LoadSnapshot(const char* path_file)
{
   return emulator_engine_->LoadSnapshot(path_file);
}

int Emulation::LoadDisk(DataContainer* container, unsigned int drive_number , bool differential_load )
{
   return emulator_engine_->LoadDisk(container, drive_number, differential_load);
}

int Emulation::LoadTape(const char* file_path)
{
   return emulator_engine_->LoadTape(file_path);
}

int Emulation::LoadTape(IContainedElement* container)
{
   return emulator_engine_->LoadTape(container);
}

int Emulation::LoadCpr(const char* file_path)
{
   return emulator_engine_->LoadCpr(file_path);
}
