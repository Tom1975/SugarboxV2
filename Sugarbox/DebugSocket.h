#pragma once

#include "Emulation.h"

class DebugSocket
{
public:
   DebugSocket(Emulation* emulation);


protected:
   Emulation* emulation_;

};