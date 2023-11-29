#pragma once
#include <filesystem>
#include <queue>

#include "Script.h"

class SCLPlayer 
{
public:
   SCLPlayer();
   virtual ~SCLPlayer();

   void LoadScript(std::filesystem::path& path);

   void AddCommand(const char* command);

   bool WaitingScript();
   void ExecuteNext();

protected:

   std::queue <Script> script_;
};
