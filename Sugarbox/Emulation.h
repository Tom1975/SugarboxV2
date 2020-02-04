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



protected:

   Motherboard* motherboard_;

   EmulatorEngine* emulator_engine_;
   CSnapshot sna_handler_;

   ConfigurationManager config_manager_;
   EmulatorSettings emulator_settings_;

   std::thread* worker_thread_;
   bool running_thread_;
};