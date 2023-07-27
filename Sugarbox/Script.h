#pragma once

#include <filesystem>

#include "DebugCommand.h"

class IRemoteCommand;

class Script 
{
public:
   Script(IRemoteCommand* command, std::vector<std::string> parameter );
   virtual ~Script();

   virtual void Execute();


protected:
   IRemoteCommand* command_;
   std::vector<std::string> parameter_;
};
