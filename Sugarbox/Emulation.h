#pragma once

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
   CSnapshot sna_handler_;

   ConfigurationManager config_manager_;

};