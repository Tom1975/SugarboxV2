#pragma once

#include "DebugCommand.h"

class IRemoteCommand;

class Script 
{
public:
   Script(IRemoteCommand* command, std::vector<std::string> parameter );
   virtual ~Script();

   void LoadScript(std::filesystem::path& path);

   virtual void Execute();


protected:
   IRemoteCommand* command_;
   std::vector<std::string> parameter_;
};
