#pragma once

#include "Emulation.h"
#include "Display.h"
#include "SFML/Graphics.hpp"

#include "Machine.h"
#include "Motherboard.h"
#include "Snapshot.h"
#include "ConfigurationManager.h"

class SugarboxApp
{
public:
   SugarboxApp();
   virtual ~SugarboxApp();

   int RunApp();

protected:
   Emulation emulation;
   CDisplay display;

};