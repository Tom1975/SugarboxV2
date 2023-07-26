#pragma once
#include <filesystem>
#include <queue>

#include "Script.h"

class SCLPlayer 
{
public:
   SCLPlayer();
   virtual ~SCLPlayer();

   void AddScript(std::filesystem::path script_path);
   void AddCommand(const char* command);

   bool WaitingScript();
   void ExecuteNext();

protected:
   std::queue <Script> script_;
};
