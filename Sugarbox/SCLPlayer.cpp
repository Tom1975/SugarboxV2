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

      std::string current_parameter;
      char current_delim = ' ';
      for (auto& c : line)
      {
         if (c == ';')
         {
            break;
         }
         if (c == current_delim)
         {
            // New 
            if (current_parameter.size() > 0)
               command_parameters.push_back(current_parameter);
            current_parameter.clear();
            current_delim = ' ';
         }
         else if (c == '\'' && current_parameter.size() == 0)
         {
            current_delim = '\'';
         }
         else if (c == '\"' && current_parameter.size() == 0)
         {
            current_delim = '\"';
         }
         else
         {
            current_parameter += c;
         }
      }
      // Comment : no more parameter to handle
      if (current_parameter.size() > 0)
         command_parameters.push_back(current_parameter);

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

