
#include "Script.h"


Script::Script(IRemoteCommand* command, std::vector<std::string> parameter) : command_(command), parameter_(parameter)
{
}

Script::~Script()
{
}

void Script::Execute()
{
   // Set the version of the script
   command_->Execute(parameter_);
}
