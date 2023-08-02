#include <fstream>

#include "SCLPlayer.h"

template <typename Out> void split(const std::string& s, char delim, Out result);

SCLPlayer::SCLPlayer()
{
   // Init commands

}

SCLPlayer::~SCLPlayer()
{
}


void SCLPlayer::LoadScript(std::filesystem::path& script_path)
{
   std::ifstream f(script_path);
   std::string line;

   while (std::getline(f, line))
   {
      // Handle line
      std::string::size_type begin = line.find_first_not_of(" \f\t\v");

      // Skip blank lines
      if (begin == std::string::npos) continue;
      // Skip commentary
      std::string::size_type end = line.find_first_of(";");
      if ( end != std::string::npos)
         line = line.substr(begin, end);

      if (line.empty()) continue;

      // Get command
      std::vector<std::string> command_parameters;
      split(line, ' ', std::back_inserter(command_parameters));

      // Look for command
      IScriptCommand* command = ScriptCommandFactory::GetCommand(command_parameters[0]);
      if (command != nullptr)
      {
         script_.push(Script(command, command_parameters));
      }

   }
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
   if(!script_.empty())
   {
      auto script = script_.front();

      script.Execute();
      script_.pop();
   }
}

