#include "SCLPlayer.h"

SCLPlayer::SCLPlayer()
{
}

SCLPlayer::~SCLPlayer()
{
}

void SCLPlayer::AddScript(std::filesystem::path script_path)
{
   
}

void SCLPlayer::AddCommand(const char* command)
{
   
}

bool SCLPlayer::WaitingScript()
{
   return script_.empty() == false;
}

void SCLPlayer::ExecuteNext()
{
   while (!script_.empty())
   {
      auto script = script_.front();

      script.Execute();
      script_.pop();
   }
}