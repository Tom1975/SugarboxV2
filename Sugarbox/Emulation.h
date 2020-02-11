#pragma once

#include "Machine.h"
#include "Motherboard.h"
#include "Snapshot.h"
#include "ConfigurationManager.h"
#include "ISound.h"

class Emulation 
{
public :
   Emulation();
   virtual ~Emulation();

   virtual void Init(IDisplay* display, ISoundFactory* sound);
   virtual void Stop();
   virtual void EmulationLoop();

   IKeyboard* GetKeyboardHandler();
   unsigned int GetSpeed();

   // Media handling
   DataContainer* CanLoad(const char* file, std::vector<MediaManager::MediaType>list_of_types = { MediaManager::MEDIA_DISK, MediaManager::MEDIA_SNA, MediaManager::MEDIA_SNR, MediaManager::MEDIA_TAPE, MediaManager::MEDIA_BIN,MediaManager::MEDIA_CPR });
   bool LoadSnr(const char* path_file);
   bool LoadBin(const char* path_file);
   bool LoadSnapshot(const char* path_file);
   int LoadDisk(DataContainer* container, unsigned int drive_number = 0, bool differential_load = true);
   int LoadTape(const char* file_path);
   int LoadTape(IContainedElement* container);
   int LoadCpr(const char* file_path);

   EmulatorEngine* GetEngine() 
   {
      return emulator_engine_;
   };

protected:

   Motherboard* motherboard_;

   EmulatorEngine* emulator_engine_;
   CSnapshot sna_handler_;

   ConfigurationManager config_manager_;
   EmulatorSettings emulator_settings_;

   std::thread* worker_thread_;
   bool running_thread_;
};