#pragma once

#include "Machine.h"
#include "Motherboard.h"
#include "Snapshot.h"
#include "ConfigurationManager.h"

class Emulation
{
public :
   Emulation();
   virtual ~Emulation();

   virtual void Init(KeyboardHandler * keyboard_handler, IDisplay* display);
   virtual void EmulationLoop();


protected:
   Motherboard* motherboard_;

   EmulatorEngine* emulator_engine_;
   CSnapshot sna_handler_;

   ConfigurationManager config_manager_;
   EmulatorSettings emulator_settings_;
};