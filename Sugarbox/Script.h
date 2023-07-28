#pragma once

#include <filesystem>

#include "DebugCommand.h"

class ScriptContext
{
public:
   ScriptContext();
   virtual ~ScriptContext();

   void Init(Emulation* emulation);

   void SetVersion(const std::string& version);

   Emulation* GetEmulation();

protected:
   Emulation* emulation_;
   std::string version_;
};

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

