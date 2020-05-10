#pragma once

#include <mutex>

#include "Machine.h"
#include "Motherboard.h"
#include "Snapshot.h"
#include "ConfigurationManager.h"
#include "ISound.h"
#include "Inotify.h"
#include "ALSoundMixer.h"
#include "Z80Desassember.h"

class INotifier
{
public:
   virtual void DiskLoaded() = 0;
};

class Emulation  : public IDirectories, IFdcNotify
{
public :
   Emulation(INotifier* notifier);
   virtual ~Emulation();

   typedef enum 
   {
      TAPE_WAV,
      TAPE_CDT_DRB,
      TAPE_CDT_CSW,
      TAPE_CSW11,
      TAPE_CSW20
   } TapeFormat;

   virtual void ChangeConfig(MachineSettings* settings);

   // IFdcNotify
   virtual void ItemLoaded(const char* disk_path, int load_ok, int drive_number);
   virtual void DiskEject();
   virtual void DiskRunning(bool on);
   virtual void TrackChanged(int nb_tracks);

   virtual void Init(IDisplay* display, ISoundFactory* sound, ALSoundMixer* sound_mixer, const char* current_path);
   virtual void Stop();
   virtual void HardReset();
   virtual void Pause();
   virtual bool EmulationRun();
   virtual void EmulationLoop();

   virtual const char* GetBaseDirectory();
   IKeyboard* GetKeyboardHandler();
   unsigned int GetSpeed();

   // Media handling
   bool IsAutoloadEnabled();
   void ToggleAutoload();
   bool IsDiskPresent(unsigned int drive);
   void InsertBlankDisk(int drive, IDisk::DiskType type);
   DataContainer* CanLoad(const char* file, std::vector<MediaManager::MediaType>list_of_types = { MediaManager::MEDIA_DISK, MediaManager::MEDIA_SNA, MediaManager::MEDIA_SNR, MediaManager::MEDIA_TAPE, MediaManager::MEDIA_BIN,MediaManager::MEDIA_CPR });
   bool LoadSnapshot(const char* file_path);
   void SaveSnapshot(const char* file_path);
   void QuickLoadsnapshot();
   void QuickSavesnapshot();
   bool LoadSnr(const char* path_file);
   void RecordSnr(const char* file_path);
   bool LoadBin(const char* path_file);
   void SaveDiskAs(unsigned int drive_number, const char* file, const FormatType* format_type);
   int LoadDisk(DataContainer* container, unsigned int drive_number = 0, bool differential_load = true);
   int LoadDisk(const char* container, unsigned int drive_number);

   void SaveTapeAs(const char* file, TapeFormat tape_format);
   int LoadTape(const char* file_path);
   int LoadTape(IContainedElement* container);
   int LoadCpr(const char* file_path);

   // Tape
   void TapeRecord();
   void TapePlay();
   void TapeFastForward();
   void TapeRewind();
   void TapePause();
   void TapeStop();

   // Debug access
   enum
   {
      DBG_NONE,
      DBG_RUN,
      DBG_RUN_FIXED_OP,
      DBG_STEP,
      DBG_BREAK,
   }debug_action_;

   void Break();
   std::vector<std::string> GetZ80Registers();
   unsigned int ReadMemory(unsigned short address, unsigned char * buffer, unsigned int size);
   void ClearBreakpoints();
   const char* GetStackType(unsigned int index);
   unsigned short GetStackShort(unsigned int index);
   int Disassemble(unsigned short address, char* buffer, int buffer_size);
   void Step();
   void Run( int nb_opcodes = 0);

   //Auto type
   void AutoType(const char* clipboard);

   EmulatorEngine* GetEngine() 
   {
      return emulator_engine_;
   };

   ConfigurationManager* GetConfigurationManager()
   {
      return &config_manager_;
   }
   

protected:
   INotifier* notifier_;

   Motherboard* motherboard_;
   Z80Desassember* disassembler_;

   EmulatorEngine* emulator_engine_;
   CSnapshot sna_handler_;

   ConfigurationManager config_manager_;
   EmulatorSettings emulator_settings_;
   ALSoundMixer * sound_mixer_;

   // Autorun
   bool autorun_;

   // Emulation control
   bool pause_;

   unsigned nb_opcode_to_run_;


   std::thread* worker_thread_;
   bool running_thread_;
   bool emulation_stopped_;

   // Thread synchronisation
   bool command_waiting_;
   std::mutex command_mutex_;
   std::string current_path_;
};