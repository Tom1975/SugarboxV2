#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <set>

#include "IConfiguration.h"

class FlagHandler
{
public:

   FlagHandler();
   virtual ~FlagHandler();

   void AddFlag(unsigned short);
   void ToggleFlag(unsigned short);
   void RemoveFlag(unsigned short);
   void RemoveAllFlags(unsigned short);

   bool IsFlagged(unsigned short);

protected:
   std::set<unsigned short> flag_list_;
};
