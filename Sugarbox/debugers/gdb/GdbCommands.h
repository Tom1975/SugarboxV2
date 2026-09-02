#pragma once

#include "../DebugCommand.h"

////////////////////////////////////////////////////////
/// query 'q'
class RemoteCommandQuery : public IRemoteCommand
{
public :
   RemoteCommandQuery();
   virtual bool Execute(std::vector<std::string>&);
   bool HandleTransfert (std::string);
   virtual std::string Help();
};

////////////////////////////////////////////////////////
/// 'v'
class RemoteCommandV : public IRemoteCommand
{
public :
   RemoteCommandV();
   virtual bool Execute(std::vector<std::string>&);
   virtual std::string Help();
};

////////////////////////////////////////////////////////
/// 'H'
class RemoteCommandH : public IRemoteCommand
{
public :
   RemoteCommandH();
   virtual bool Execute(std::vector<std::string>&);
   virtual std::string Help();
};

////////////////////////////////////////////////////////
/// 'c'
class RemoteCommandC : public IRemoteCommand
{
public :
   RemoteCommandC();
   virtual bool Execute(std::vector<std::string>&);
   virtual std::string Help();
};

////////////////////////////////////////////////////////
/// '?'
class RemoteCommandAsk : public IRemoteCommand
{
public :
   RemoteCommandAsk();
   virtual bool Execute(std::vector<std::string>&);
   virtual std::string Help();
};


////////////////////////////////////////////////////////
/// 'g'
class RemoteCommandStack : public IRemoteCommand
{
public :
   RemoteCommandStack();
   virtual bool Execute(std::vector<std::string>&);
   virtual std::string Help();
};