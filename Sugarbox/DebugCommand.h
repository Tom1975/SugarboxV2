#pragma once
#include <string>
#include <deque>
#include "Emulation.h"

////////////////////////////////////////////////////////
// Command Interface
class ICommandResponse
{
public:
   virtual void SendResponse(const char*) = 0;
   virtual void SendEoL() = 0;
   virtual void EnterCpuStep() = 0;
};

////////////////////////////////////////////////////////
// Remote command interface
class IRemoteCommand
{
public:
   void InitCommand(ICommandResponse*callback, Emulation* emulation)
   {
      callback_ = callback;
      emulation_ = emulation;
   }

   virtual bool Execute(std::deque<std::string>&) = 0;
   virtual std::string Help() = 0;
protected:
   ICommandResponse* callback_;
   Emulation* emulation_;
};

////////////////////////////////////////////////////////
/// About
class RemoteCommandAbout : public IRemoteCommand
{
public :
   virtual bool Execute(std::deque<std::string>&);
   virtual std::string Help();
};

////////////////////////////////////////////////////////
///  CPU Step
class RemoteCommandCpuStep : public IRemoteCommand
{
public:
   virtual bool Execute(std::deque<std::string>&);
   virtual std::string Help() { return "Run single opcode cpu step."; };
};

////////////////////////////////////////////////////////
///  Disassemble
class RemoteCommandDisassemble : public IRemoteCommand
{
public:
   virtual bool Execute(std::deque<std::string>&);
   virtual std::string Help(){ return "Disassemble at address. If no address specified, disassemble from PC register. If no lines specified, disassembles one line"; }
};

////////////////////////////////////////////////////////
///  ExtendedStack
class RemoteCommandExtendedStack : public IRemoteCommand
{
public:
   virtual bool Execute(std::deque<std::string>&);
   virtual std::string Help() { return "Sets extended stack parameters, which allows you to see what kind of values are in the stack. Action and parameters are the following:\n	get     n[index]  Get n values.The index default value is the SP register\n"; }
};

////////////////////////////////////////////////////////
///  ExtendedStack
class RemoteCommandClearMembreakpoints : public IRemoteCommand
{
public:
   virtual bool Execute(std::deque<std::string>&);
   virtual std::string Help() { return "Clear all memory breakpoints"; }
};
