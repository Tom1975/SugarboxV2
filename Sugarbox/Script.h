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


class CommandGenericType : public IScriptCommand
{
public:
   virtual int GetNextKey(std::string& line, int index, std::vector<char>& next);
   virtual void TypeLineOfText(std::string& line);
   virtual void Wait(unsigned int nb_us);

};

////////////////////////////////////////////////////////
///  CSL Commands
///
class CommandCslVersion : public CommandGenericType
{
public:
   virtual bool Execute(std::vector<std::string>&);
};

class CommandReset : public CommandGenericType
{
public:
   virtual bool Execute(std::vector<std::string>&);
};

class CommandCrtcSelect : public CommandGenericType
{
public:
   virtual bool Execute(std::vector<std::string>&);
};

class CommandDiskInsert : public CommandGenericType
{
public:
   virtual bool Execute(std::vector<std::string>&);
};

class CommandDiskDir : public CommandGenericType
{
public:
   virtual bool Execute(std::vector<std::string>&);
};

class CommandTapeInsert : public CommandGenericType
{
public:
   virtual bool Execute(std::vector<std::string>&);
};

class CommandTapeDir : public CommandGenericType
{
public:
   virtual bool Execute(std::vector<std::string>&);
};

class CommandTapePlay : public CommandGenericType
{
public:
   virtual bool Execute(std::vector<std::string>&);
};

class CommandTapeStop : public CommandGenericType
{
public:
   virtual bool Execute(std::vector<std::string>&);
};

class CommandTapeRewind : public CommandGenericType
{
public:
   virtual bool Execute(std::vector<std::string>&);
};

class CommandSnapshotLoad : public CommandGenericType
{
public:
   virtual bool Execute(std::vector<std::string>&);
};

class CommandSnapshotDir : public CommandGenericType
{
public:
   virtual bool Execute(std::vector<std::string>&);
};

class CommandKeyDelay: public CommandGenericType
{
public:
   virtual bool Execute(std::vector<std::string>&);
};

class CommandKeyOutput : public CommandGenericType
{
public:
   virtual bool Execute(std::vector<std::string>&);
};

class CommandKeyFromFile : public CommandGenericType
{
public:
   virtual bool Execute(std::vector<std::string>&);
};

class CommandWait : public CommandGenericType
{
public:
   virtual bool Execute(std::vector<std::string>&);
};

class CommandWaitDriveOnOff : public CommandGenericType
{
public:
   virtual bool Execute(std::vector<std::string>&);
};

class CommandWaitVSyncOffOn: public CommandGenericType
{
public:
   virtual bool Execute(std::vector<std::string>&);
};

class CommandScreenshotName : public CommandGenericType
{
public:
   virtual bool Execute(std::vector<std::string>&);
};

class CommandScreenshotDir : public CommandGenericType
{
public:
   virtual bool Execute(std::vector<std::string>&);
};

class CommandScreenshot : public CommandGenericType
{
public:
   virtual bool Execute(std::vector<std::string>&);
};

class CommandSnapshotName : public CommandGenericType
{
public:
   virtual bool Execute(std::vector<std::string>&);
};

class CommandSnapshot : public CommandGenericType
{
public:
   virtual bool Execute(std::vector<std::string>&);
};

class CommandCslLoad: public CommandGenericType
{
public:
   virtual bool Execute(std::vector<std::string>&);
};