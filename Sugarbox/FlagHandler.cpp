#include <cstring>

#include "FlagHandler.h"

/////////////////////////////////////////////////////////////
/// Helper functions
FlagHandler::FlagHandler()
{
}

FlagHandler::~FlagHandler()
{
}

void FlagHandler::AddFlag(unsigned short addr)
{
   flag_list_.insert(addr);
}

void FlagHandler::ToggleFlag(unsigned short addr)
{
   if ( flag_list_.find(addr) != flag_list_.end())
   {
      flag_list_.erase(addr);
   }
   else
   {
      flag_list_.insert(addr);
   }
}

void FlagHandler::RemoveFlag(unsigned short addr)
{
   flag_list_.erase(addr);
}

void FlagHandler::RemoveAllFlags(unsigned short addr)
{
   flag_list_.clear();
}

bool FlagHandler::IsFlagged(unsigned short addr)
{
   return flag_list_.find(addr) != flag_list_.end();
}