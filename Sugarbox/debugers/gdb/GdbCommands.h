#pragma once

#include "../DebugCommand.h"

////////////////////////////////////////////////////////
/// query 'q'
class RemoteCommandQuery : public IRemoteCommand
{
public :
   RemoteCommandQuery();
   virtual bool Execute(std::vector<std::string>&);
   virtual std::string Help();
};

