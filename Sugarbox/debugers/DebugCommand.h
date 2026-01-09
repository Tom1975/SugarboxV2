#pragma once
#include <map>
#include <string>
#include <vector>

class Emulation;

////////////////////////////////////////////////////////
// Command Interface
class ICommandResponse
{
public:
   virtual void SendResponse(const char*) = 0;
   virtual void SendEoL() = 0;

   virtual void EnterCpuStep() = 0;
   virtual void ExitCpuStep() = 0;

   virtual void Log(const char*) = 0;
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

   virtual bool Execute(std::vector<std::string>&) = 0;
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
   virtual bool Execute(std::vector<std::string>&);
   virtual std::string Help();
};

////////////////////////////////////////////////////////
/// Break
class RemoteCommandBreak : public IRemoteCommand
{
public:
   virtual bool Execute(std::vector<std::string>&);
   virtual std::string Help();
};

////////////////////////////////////////////////////////
///  CPU Step
class RemoteCommandCpuStep : public IRemoteCommand
{
public:
   virtual bool Execute(std::vector<std::string>&);
   virtual std::string Help() { return "Run single opcode cpu step."; };
};

////////////////////////////////////////////////////////
///  Disassemble
class RemoteCommandDisassemble : public IRemoteCommand
{
public:
   virtual bool Execute(std::vector<std::string>&);
   virtual std::string Help(){ return "Disassemble at address. If no address specified, disassemble from PC register. If no lines specified, disassembles one line"; }
};

////////////////////////////////////////////////////////
///  ExtendedStack
class RemoteCommandExtendedStack : public IRemoteCommand
{
public:
   virtual bool Execute(std::vector<std::string>&);
   virtual std::string Help() { return "Sets extended stack parameters, which allows you to see what kind of values are in the stack. Action and parameters are the following:\n	get     n[index]  Get n values.The index default value is the SP register\n"; }
};

////////////////////////////////////////////////////////
///  Clear memory breakpoints
class RemoteCommandClearMembreakpoints : public IRemoteCommand
{
public:
   virtual bool Execute(std::vector<std::string>&);
   virtual std::string Help() { return "Clear all memory breakpoints"; }
};

////////////////////////////////////////////////////////
///  Disable breakpoint
class RemoteCommandDisableBreakpoint: public IRemoteCommand
{
public:
   virtual bool Execute(std::vector<std::string>&);
   virtual std::string Help() { return "Disable specific breakpoint"; }
};

////////////////////////////////////////////////////////
///  Disable breakpoints
class RemoteCommandDisableBreakpoints : public IRemoteCommand
{
public:
   virtual bool Execute(std::vector<std::string>&);
   virtual std::string Help() { return "Disable breakpoints"; }
};

////////////////////////////////////////////////////////
///  Disable breakpoint
class RemoteCommandEnableBreakpoint : public IRemoteCommand
{
public:
   virtual bool Execute(std::vector<std::string>&);
   virtual std::string Help() { return "Enable specific breakpoint"; }
};

////////////////////////////////////////////////////////
///  Disable breakpoint
class RemoteCommandEnableBreakpoints : public IRemoteCommand
{
public:
   virtual bool Execute(std::vector<std::string>&);
   virtual std::string Help() { return "Enable breakpoints"; }
};

////////////////////////////////////////////////////////
///  Get CPU Frequency
class RemoteCommandGetCPUFrequency : public IRemoteCommand
{
public:
   virtual bool Execute(std::vector<std::string>&);
   virtual std::string Help() { return "Get cpu frequency in hz"; }
};

////////////////////////////////////////////////////////
///  Get current machine
class RemoteCommandGetCurrentMachine : public IRemoteCommand
{
public:
   virtual bool Execute(std::vector<std::string>&);
   virtual std::string Help() { return "Returns current machine name"; }
};

////////////////////////////////////////////////////////
///  Get Registers
class RemoteCommandGetRegisters : public IRemoteCommand
{
public:
   virtual bool Execute(std::vector<std::string>&);
   virtual std::string Help() { return "Get CPU registers"; }
}; 

////////////////////////////////////////////////////////
///  Get version
class RemoteCommandGetVersion : public IRemoteCommand
{
public:
   virtual bool Execute(std::vector<std::string>&);
   virtual std::string Help() { return "Shows emulator version"; }
};

////////////////////////////////////////////////////////
///  Hard Reset
class RemoteCommandHardReset : public IRemoteCommand
{
public:
   virtual bool Execute(std::vector<std::string>&);
   virtual std::string Help() { return "Hard resets the machine"; }
};

////////////////////////////////////////////////////////
///  Read Memory
class RemoteCommandReadMemory : public IRemoteCommand
{
public:
   virtual bool Execute(std::vector<std::string>&);
   virtual std::string Help() { return "Dumps memory at address"; }
};

////////////////////////////////////////////////////////
///  Run
class RemoteCommandRun : public IRemoteCommand
{
public:
   virtual bool Execute(std::vector<std::string>&);
   virtual std::string Help() { return "Run cpu when on cpu step mode. Returns when a breakpoint is fired."; }
};

////////////////////////////////////////////////////////
///  RemoteCommandSetBreakpoint
class RemoteCommandSetBreakpoint : public IRemoteCommand
{
public:
   virtual bool Execute(std::vector<std::string>&);
   virtual std::string Help() { return "Sets a breakpoint at desired index entry with condition. If no condition set, breakpoint will be handled as disabled"; }
}; 


