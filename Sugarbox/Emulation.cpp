
#include <chrono>
#include <thread>
#include <sstream>

#include "Emulation.h"
#include "wav.h"

// Wav registered

#define SND_SEEK_SHORT     0
#define SND_SEEK_LONG      1
#define SND_INSERT_DISK    2
#define SND_EJECT_DISK     3
#define SND_MOTOR_ON       4

//////////////////////////////////////////////
/// ctor/dtor
Emulation::Emulation(INotifier* notifier) :
   no_debug_(true),
   notifier_(notifier),
   motherboard_(nullptr), 
   sna_handler_(nullptr),
   running_thread_(false),
   pause_(false),
   break_(false),
   worker_thread_(nullptr),
   command_waiting_(false),
   sound_mixer_(nullptr),
   autorun_(true),
   emulation_stopped_(false)
{
}

Emulation::~Emulation()
{
   // End emulation
   Stop();
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

void Emulation::Init( IDisplay* display, ISoundFactory* sound, ALSoundMixer* sound_mixer, const char* current_path)
{
   sound_mixer_ = sound_mixer;
   current_path_ = current_path;
   emulator_settings_.Init(&config_manager_, sound);
   emulator_settings_.Load("Sugarbox.ini");

   emulator_engine_ = new EmulatorEngine();
   emulator_engine_->SetDirectories(this);
   emulator_engine_->SetConfigurationManager(&config_manager_);
   emulator_engine_->Init(display, nullptr);

   emulator_engine_->SetDefaultConfiguration();
   emulator_engine_->SetSettings(emulator_settings_);
   emulator_engine_->SetNotifier(this);

   emulator_engine_->LoadConfiguration("Current", "Sugarbox.ini");
   emulator_engine_->GetMem()->Initialisation();
   // Update computer
   emulator_engine_->Reinit();

   running_thread_ = true;
   worker_thread_ = new std::thread(RunLoop, this);

   sound_mixer_->AddWav(SND_SEEK_SHORT, seek_short_wav, sizeof(seek_short_wav));
   sound_mixer_->AddWav(SND_SEEK_LONG, seek_long_wav, sizeof(seek_short_wav));
   sound_mixer_->AddWav(SND_INSERT_DISK, insert_wav, sizeof(seek_short_wav));
   sound_mixer_->AddWav(SND_EJECT_DISK, eject_wav, sizeof(seek_short_wav));
   sound_mixer_->AddWav(SND_MOTOR_ON, drive_mo_wav, sizeof(seek_short_wav));

   disassembler_ = new Z80Desassember(emulator_engine_);
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
      if (!pause_ && !break_ && emulator_engine_->IsRunning())
      {
         emulator_engine_->RunTimeSlice(no_debug_);
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
   emulation_stopped_ = true;
}

void Emulation::Pause()
{
   pause_ = !pause_;
}

bool Emulation::EmulationRun()
{
   return pause_==false;
}

void Emulation::ChangeConfig(MachineSettings* settings)
{
   command_waiting_ = true;
   const std::lock_guard<std::mutex> lock(command_mutex_);
   emulator_engine_->ChangeSettings(settings);
   emulator_engine_->OnOff();
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

bool Emulation::LoadSnapshot(const char* file_path)
{
   command_waiting_ = true;
   const std::lock_guard<std::mutex> lock(command_mutex_);
   return emulator_engine_->LoadSnapshot(file_path);
}

void Emulation::SaveSnapshot(const char* file_path)
{
   command_waiting_ = true;
   const std::lock_guard<std::mutex> lock(command_mutex_);
   emulator_engine_->SaveSnapshot(file_path);
}

void Emulation::RecordSnr(const char* file_path)
{
   command_waiting_ = true;
   const std::lock_guard<std::mutex> lock(command_mutex_);
   emulator_engine_->StartRecord(file_path);
}

void Emulation::QuickLoadsnapshot()
{
   command_waiting_ = true;
   const std::lock_guard<std::mutex> lock(command_mutex_);
   emulator_engine_->QuickLoadsnapshot();

}

void Emulation::QuickSavesnapshot()
{
   command_waiting_ = true;
   const std::lock_guard<std::mutex> lock(command_mutex_);
   emulator_engine_->QuickSavesnapshot();

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

void Emulation::SaveTapeAs(const char* file, TapeFormat tape_format)
{
   command_waiting_ = true;
   const std::lock_guard<std::mutex> lock(command_mutex_);

   CTape* tape = emulator_engine_->GetTape();
   if (tape != nullptr)
   {
      switch (tape_format)
      {
      case TAPE_WAV:
         tape->SaveAsWav(file);
         break;
      case TAPE_CDT_DRB:
         tape->SaveAsCdtDrb(file);
         break;
      case TAPE_CDT_CSW:
         tape->SaveAsCdtCSW(file);
         break;
      case TAPE_CSW11:
         tape->SaveAsCSW(file, 1, 1);
         break;
      case TAPE_CSW20:
         tape->SaveAsCSW(file, 2, 2);
         break;
      }
   }
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

bool Emulation::IsAutoloadEnabled()
{
   return autorun_;
}

void Emulation::ToggleAutoload()
{
   autorun_ = !autorun_;
}

void Emulation::AutoType(const char* clipboard)
{
   // todo
   emulator_engine_->Paste(clipboard);
}

void Emulation::ItemLoaded(const char* disk_path, int load_ok, int drive_number)
{
   // Register path (for tooltip display)

   // Play insert disk soiund
   sound_mixer_->PlayWav(SND_INSERT_DISK);

   // Suppervisor should be advised of it
   notifier_->DiskLoaded();

   // Autoload ?
   // todo : skip startup disk inserted ?
   if (autorun_ && load_ok == 0 && drive_number != -1)
   {
      char fileToLoad[16] = { 0 };
      switch (emulator_engine_->GetFDC()->GetAutorun(drive_number, fileToLoad, 16))
      {
      case IDisk::AUTO_CPM:
      {
         emulator_engine_->Paste("|CPM");
         emulator_engine_->Paste("\r");
         break;
      }
      case IDisk::AUTO_FILE:
      {
         char wcsCmdLine[256];
         sprintf (wcsCmdLine, "RUN\"%s", fileToLoad);
         emulator_engine_->Paste(wcsCmdLine);
         emulator_engine_->Paste("\r");
      }
      break;

      default:
         break;
      };
   }
}

void Emulation::DiskEject()
{
   // Play "disk ejection" sound
   sound_mixer_->PlayWav(SND_EJECT_DISK);
}

void Emulation::DiskRunning(bool on)
{
   sound_mixer_->PlayWav(SND_MOTOR_ON);
}

void Emulation::TrackChanged(int nb_tracks)
{
   // play the "insert disk" wave
   if (nb_tracks < 20)
      sound_mixer_->PlayWav(SND_SEEK_SHORT);
   else
      sound_mixer_->PlayWav(SND_SEEK_LONG);
      
}

void Emulation::Break()
{
   no_debug_ = false;
   break_ = true;
   emulator_engine_->SetRun(false);
}

std::vector<std::string> Emulation::GetZ80Registers()
{
   std::vector<std::string> reg_list;
   Z80 * z80 = emulator_engine_->GetProc();

   char reg_buffer[24];

   sprintf(reg_buffer, "PC=%4.4X", z80->GetPC());
   reg_list.push_back(reg_buffer);
   sprintf(reg_buffer, "SP=%4.4X", z80->sp_);
   reg_list.push_back(reg_buffer);
   sprintf(reg_buffer, "AF=%4.4X", z80->af_);
   reg_list.push_back(reg_buffer);
   sprintf(reg_buffer, "BC=%4.4X", z80->bc_);
   reg_list.push_back(reg_buffer);
   sprintf(reg_buffer, "HL=%4.4X", z80->hl_);
   reg_list.push_back(reg_buffer);
   sprintf(reg_buffer, "DE=%4.4X", z80->de_);
   reg_list.push_back(reg_buffer);
   sprintf(reg_buffer, "IX=%4.4X", z80->ix_);
   reg_list.push_back(reg_buffer);
   sprintf(reg_buffer, "IY=%4.4X", z80->iy_);
   reg_list.push_back(reg_buffer);
   sprintf(reg_buffer, "AF'=%4.4X", z80->af_p_);
   reg_list.push_back(reg_buffer);
   sprintf(reg_buffer, "BC'=%4.4X", z80->bc_p_);
   reg_list.push_back(reg_buffer);
   sprintf(reg_buffer, "HL'=%4.4X", z80->hl_p_);
   reg_list.push_back(reg_buffer);
   sprintf(reg_buffer, "DE'=%4.4X", z80->de_p_);
   reg_list.push_back(reg_buffer);
   sprintf(reg_buffer, "I=%2.2X", z80->ir_.b.h);
   reg_list.push_back(reg_buffer);
   sprintf(reg_buffer, "R=%2.2X", z80->ir_.b.l);
   reg_list.push_back(reg_buffer);

   sprintf(reg_buffer, "F=%c%c%c%c%c%c%c%c",
      (z80->af_.b.l&SF) ? 'S' : '-',
      (z80->af_.b.l&ZF) ? 'Z' : '-',
      (z80->af_.b.l&YF) ? '5' : '-',
      (z80->af_.b.l&HF) ? 'H' : '-',
      (z80->af_.b.l&XF) ? '3' : '-',
      (z80->af_.b.l&PF) ? 'P' : '-',
      (z80->af_.b.l&NF) ? 'N' : '-',
      (z80->af_.b.l&CF) ? 'C' : '-'
   ) ;
   reg_list.push_back(reg_buffer);

   sprintf(reg_buffer, "F=%c%c%c%c%c%c%c%c",
      (z80->af_p_.b.l&SF) ? 'S' : '-',
      (z80->af_p_.b.l&ZF) ? 'Z' : '-',
      (z80->af_p_.b.l&YF) ? '5' : '-',
      (z80->af_p_.b.l&HF) ? 'H' : '-',
      (z80->af_p_.b.l&XF) ? '3' : '-',
      (z80->af_p_.b.l&PF) ? 'P' : '-',
      (z80->af_p_.b.l&NF) ? 'N' : '-',
      (z80->af_p_.b.l&CF) ? 'C' : '-'
   );
   reg_list.push_back(reg_buffer);

   sprintf(reg_buffer, "MEMPTR=%4.4X", z80->mem_ptr_);
   reg_list.push_back(reg_buffer);

   sprintf(reg_buffer, "IM=%d", z80->interrupt_mode_);
   reg_list.push_back(reg_buffer);
   
   sprintf(reg_buffer, "IFF=%c%c", z80->iff1_?'1':'-', z80->iff2_ ? '2' : '-');
   reg_list.push_back(reg_buffer);
   
   sprintf(reg_buffer, "Q=%2.2X", z80->q_);
   reg_list.push_back(reg_buffer);

   return reg_list;
}
unsigned int Emulation::ReadMemory( unsigned short address, unsigned char * buffer, unsigned int size)
{
   command_waiting_ = true;
   const std::lock_guard<std::mutex> lock(command_mutex_);
   emulator_engine_->GetMem()->GetDebugValue(buffer, address, size, Memory::MEM_READ, 0);
   return size;
}

void Emulation::ClearBreakpoints()
{
   command_waiting_ = true;
   const std::lock_guard<std::mutex> lock(command_mutex_);
   emulator_engine_->ClearBreakpoints();
}

const char* Emulation::GetStackType(unsigned int index)
{
   // depend on the type of call. TBD !
   // can be : 'call' or 'rst', or 'interrupt'
   return "pushed";
}

unsigned short Emulation::GetStackShort(unsigned int index)
{
   // address : 
   unsigned short stack_address = emulator_engine_->GetProc()->sp_ += index * 2;
   return emulator_engine_->GetMem()->GetWord(stack_address);
}

void Emulation::Disassemble(unsigned short address, char* buffer, int buffer_size)
{
   char mnemonic[16];
   char argument[16];
   disassembler_->DasmMnemonic(address, mnemonic, argument);

   std::snprintf(buffer, buffer_size, "%s %s", mnemonic, argument);
}

void Emulation::Step()
{
   emulator_engine_->SetStepIn(true);
   emulator_engine_->SetRun(true);
   break_ = false;
   no_debug_ = false;
}