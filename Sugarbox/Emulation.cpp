
#include <chrono>
#include <thread>

#include "Emulation.h"


//////////////////////////////////////////////
/// ctor/dtor
Emulation::Emulation() :
   motherboard_(nullptr), 
   sna_handler_(nullptr),
   running_thread_(false),
   pause_(false),
   worker_thread_(nullptr),
   command_waiting_(false)
{
}

Emulation::~Emulation()
{
   delete motherboard_;
}

//////////////////////////////////////////////
///

const char* Emulation::GetBaseDirectory()
{
   return current_path_.c_str();
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

void Emulation::Init( IDisplay* display, ISoundFactory* sound, const char* current_path)
{
   current_path_ = current_path;
   emulator_settings_.Init(&config_manager_, sound);
   emulator_settings_.Load("Sugarbox.ini");

   emulator_engine_ = new EmulatorEngine();
   emulator_engine_->SetDirectories(this);
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
   const std::lock_guard<std::mutex> lock(command_mutex_);

   while (running_thread_)
   {
      if (!pause_)
      {
         emulator_engine_->RunTimeSlice(true);
      }
      else
      {
         std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }

      // Command waiting ? 
      if (command_waiting_)
      {
         command_mutex_.unlock();
         
         std::this_thread::sleep_for(std::chrono::milliseconds(1));
         command_waiting_ = false;
         command_mutex_.lock();
      }
   }
   emulator_engine_->Stop();
}

void Emulation::Pause()
{
   pause_ = !pause_;
}

bool Emulation::EmulationRun()
{
   return pause_==false;
}

void Emulation::HardReset()
{
   command_waiting_ = true;
   const std::lock_guard<std::mutex> lock(command_mutex_);
   return emulator_engine_->OnOff();
}

DataContainer* Emulation::CanLoad(const char* file, std::vector<MediaManager::MediaType>list_of_types)
{
   return emulator_engine_->CanLoad(file, list_of_types);
}

bool Emulation::LoadSnr(const char* path_file)
{
   command_waiting_ = true;
   const std::lock_guard<std::mutex> lock(command_mutex_);
   return emulator_engine_->LoadSnr(path_file);
}

bool Emulation::LoadBin(const char* path_file)
{
   command_waiting_ = true;
   const std::lock_guard<std::mutex> lock(command_mutex_);
   return emulator_engine_->LoadBin(path_file);
}

bool Emulation::LoadSnapshot(const char* path_file)
{
   command_waiting_ = true;
   const std::lock_guard<std::mutex> lock(command_mutex_);
   return emulator_engine_->LoadSnapshot(path_file);
}

bool Emulation::IsDiskPresent(unsigned int drive)
{
   command_waiting_ = true;
   const std::lock_guard<std::mutex> lock(command_mutex_);
   return emulator_engine_->IsDiskPresent(drive);
}

void Emulation::InsertBlankDisk(int drive, IDisk::DiskType type)
{
   command_waiting_ = true;
   const std::lock_guard<std::mutex> lock(command_mutex_);
   emulator_engine_->InsertBlankDisk(drive, type );
}

void Emulation::SaveDiskAs(unsigned int drive_number, const char* file, const FormatType* format_type)
{
   command_waiting_ = true;
   const std::lock_guard<std::mutex> lock(command_mutex_);
   emulator_engine_->SaveDiskAs(drive_number, file, format_type);
}

int Emulation::LoadDisk(DataContainer* container, unsigned int drive_number , bool differential_load )
{
   command_waiting_ = true;
   const std::lock_guard<std::mutex> lock(command_mutex_);
   return emulator_engine_->LoadDisk(container, drive_number, differential_load);
}

int Emulation::LoadDisk(const char* container, unsigned int drive_number)
{
   command_waiting_ = true;
   const std::lock_guard<std::mutex> lock(command_mutex_);
   return emulator_engine_->LoadDisk(container, drive_number);
}

int Emulation::LoadTape(const char* file_path)
{
   command_waiting_ = true;
   const std::lock_guard<std::mutex> lock(command_mutex_);
   return emulator_engine_->LoadTape(file_path);
}

int Emulation::LoadTape(IContainedElement* container)
{
   command_waiting_ = true;
   const std::lock_guard<std::mutex> lock(command_mutex_);
   return emulator_engine_->LoadTape(container);
}

int Emulation::LoadCpr(const char* file_path)
{
   command_waiting_ = true;
   const std::lock_guard<std::mutex> lock(command_mutex_);
   return emulator_engine_->LoadCpr(file_path);
}


void Emulation::TapeRecord()
{
   command_waiting_ = true;
   const std::lock_guard<std::mutex> lock(command_mutex_);
   return emulator_engine_->GetTape()->Record();
}

void Emulation::TapePlay()
{
   command_waiting_ = true;
   const std::lock_guard<std::mutex> lock(command_mutex_);
   return emulator_engine_->GetTape()->Play();
}

void Emulation::TapeFastForward()
{
   command_waiting_ = true;
   const std::lock_guard<std::mutex> lock(command_mutex_);
   return emulator_engine_->GetTape()->FastForward();
}

void Emulation::TapeRewind()
{
   command_waiting_ = true;
   const std::lock_guard<std::mutex> lock(command_mutex_);
   return emulator_engine_->GetTape()->Rewind();
}

void Emulation::TapePause()
{
   command_waiting_ = true;
   const std::lock_guard<std::mutex> lock(command_mutex_);
   return emulator_engine_->GetTape()->Pause();
}

void Emulation::TapeStop()
{
   command_waiting_ = true;
   const std::lock_guard<std::mutex> lock(command_mutex_);
   return emulator_engine_->GetTape()->StopEject();
}

