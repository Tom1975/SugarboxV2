#pragma once
#include <string>
#include <deque>

// Command Interface
class ICommandResponse
{
public:
   virtual void SendResponse(const char*) = 0;
   virtual void SendEoL() = 0;
};

class IRemoteCommand
{
public:
   void InitCommand(ICommandResponse*callback)
   {
      callback_ = callback;
   }

   virtual bool Execute(std::deque<std::string>&) = 0;
   virtual std::string Help() = 0;
protected:
   ICommandResponse* callback_;
};


class RemoteCommandAbout : public IRemoteCommand
{
public :
   virtual bool Execute(std::deque<std::string>&);
   virtual std::string Help();
};