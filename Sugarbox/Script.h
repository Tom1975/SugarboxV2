#pragma once

#include <filesystem>

#include "DebugCommand.h"
#include "ScriptContext.h"

class IScriptCommand
{
public:
   void InitCommand(ScriptContext* context)
   {
      context_ = context;
   }

   virtual bool Execute(std::vector<std::string>&) = 0;

protected:
   ScriptContext* context_;

};

class Script 
{
public:
   Script(IScriptCommand* command, std::vector<std::string> parameter );
   virtual ~Script();

   virtual void Execute();


protected:
   IScriptCommand* command_;
   std::vector<std::string> parameter_;
};

class ScriptCommandFactory
{
public:
   static void InitFactory(ScriptContext* context);

   static IScriptCommand* GetCommand(std::string& command_name);

protected:
   static void AddCommand(IScriptCommand* action, std::initializer_list<std::string >commands);

   static ScriptContext* context_;
   static std::map<std::string, IScriptCommand* > function_map_;
};


////////////////////////////////////////////////////////
///  CSL Commands
///
class CommandCslVersion : public IScriptCommand
{
public:
   virtual bool Execute(std::vector<std::string>&);
};

class CommandReset : public IScriptCommand
{
public:
   virtual bool Execute(std::vector<std::string>&);
};

class CommandCrtcSelect : public IScriptCommand
{
public:
   virtual bool Execute(std::vector<std::string>&);
};

class CommandDiskInsert : public IScriptCommand
{
public:
   virtual bool Execute(std::vector<std::string>&);
};

class CommandDiskDir : public IScriptCommand
{
public:
   virtual bool Execute(std::vector<std::string>&);
};

class CommandTapeInsert : public IScriptCommand
{
public:
   virtual bool Execute(std::vector<std::string>&);
};

class CommandTapeDir : public IScriptCommand
{
public:
   virtual bool Execute(std::vector<std::string>&);
};

class CommandTapePlay : public IScriptCommand
{
public:
   virtual bool Execute(std::vector<std::string>&);
};

class CommandTapeStop : public IScriptCommand
{
public:
   virtual bool Execute(std::vector<std::string>&);
};

class CommandTapeRewind : public IScriptCommand
{
public:
   virtual bool Execute(std::vector<std::string>&);
};

class CommandSnapshotLoad : public IScriptCommand
{
public:
   virtual bool Execute(std::vector<std::string>&);
};

class CommandSnapshotDir : public IScriptCommand
{
public:
   virtual bool Execute(std::vector<std::string>&);
};

class CommandKeyDelay: public IScriptCommand
{
public:
   virtual bool Execute(std::vector<std::string>&);
};

class CommandKeyOutput : public IScriptCommand
{
public:
   virtual bool Execute(std::vector<std::string>&);
};

class CommandKeyFromFile : public IScriptCommand
{
public:
   virtual bool Execute(std::vector<std::string>&);
};

class CommandWait : public IScriptCommand
{
public:
   virtual bool Execute(std::vector<std::string>&);
};

class CommandWaitDriveOnOff : public IScriptCommand
{
public:
   virtual bool Execute(std::vector<std::string>&);
};

class CommandWaitVSyncOffOn: public IScriptCommand
{
public:
   virtual bool Execute(std::vector<std::string>&);
};

class CommandScreenshotName : public IScriptCommand
{
public:
   virtual bool Execute(std::vector<std::string>&);
};

class CommandScreenshotDir : public IScriptCommand
{
public:
   virtual bool Execute(std::vector<std::string>&);
};

class CommandScreenshot : public IScriptCommand
{
public:
   virtual bool Execute(std::vector<std::string>&);
};

class CommandSnapshotName : public IScriptCommand
{
public:
   virtual bool Execute(std::vector<std::string>&);
};

class CommandSnapshot : public IScriptCommand
{
public:
   virtual bool Execute(std::vector<std::string>&);
};

class CommandCslLoad: public IScriptCommand
{
public:
   virtual bool Execute(std::vector<std::string>&);
};