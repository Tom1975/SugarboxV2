
#include "Script.h"

#include "Emulation.h"


ScriptContext::ScriptContext() : emulation_(nullptr)
{
   
}

ScriptContext::~ScriptContext()
{
   
}

void ScriptContext::Init(Emulation* emulation)
{
   emulation_ = emulation;
}

void ScriptContext::SetVersion(const std::string& version)
{
   version_ = version;
}

Emulation* ScriptContext::GetEmulation()
{
   return emulation_;
}


Script::Script(IScriptCommand* command, std::vector<std::string> parameter) : command_(command), parameter_(parameter)
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

////////////////////////////////////////////////////////
/// Factory
///
ScriptContext* ScriptCommandFactory::context_= nullptr;

std::map<std::string, IScriptCommand* > ScriptCommandFactory::function_map_ = {};

void ScriptCommandFactory::InitFactory(ScriptContext* context)
{
   context_ = context;

   AddCommand(new CommandCslVersion(), { "csl_version" });
   AddCommand(new CommandReset(), { "reset" });
   /*
   AddCommand(new RemoteCommandSetBreakpoint(), { "crtc_select" });
   AddCommand(new RemoteCommandSetBreakpoint(), { "disk_insert" });
   AddCommand(new RemoteCommandSetBreakpoint(), { "disk_dir" });
   AddCommand(new RemoteCommandSetBreakpoint(), { "tape_insert" });
   AddCommand(new RemoteCommandSetBreakpoint(), { "tape_dir" });
   AddCommand(new RemoteCommandSetBreakpoint(), { "tape_play" });
   AddCommand(new RemoteCommandSetBreakpoint(), { "tape_stop" });
   AddCommand(new RemoteCommandSetBreakpoint(), { "tape_rewind" });
   AddCommand(new RemoteCommandSetBreakpoint(), { "snapshot_load" });
   AddCommand(new RemoteCommandSetBreakpoint(), { "snapshot_dir" });
   AddCommand(new RemoteCommandSetBreakpoint(), { "key_delay" });
   AddCommand(new RemoteCommandSetBreakpoint(), { "key_output" });
   AddCommand(new RemoteCommandSetBreakpoint(), { "key_from_file" });
   AddCommand(new RemoteCommandSetBreakpoint(), { "wait" });
   AddCommand(new RemoteCommandSetBreakpoint(), { "wait_driveonoff" });
   AddCommand(new RemoteCommandSetBreakpoint(), { "wait_vsyncoffon" });
   AddCommand(new RemoteCommandSetBreakpoint(), { "screenshot_name" });
   AddCommand(new RemoteCommandSetBreakpoint(), { "screenshot_dir" });
   AddCommand(new RemoteCommandSetBreakpoint(), { "screenshot" });
   AddCommand(new RemoteCommandSetBreakpoint(), { "snapshot_name" });
   AddCommand(new RemoteCommandSetBreakpoint(), { "snapshot" });
   AddCommand(new RemoteCommandSetBreakpoint(), { "csl_load" });*/

}

IScriptCommand* ScriptCommandFactory::GetCommand(std::string& command_name)
{
   IScriptCommand* current_command = nullptr;
   if (function_map_.find(command_name) != function_map_.end())
   {
      current_command = function_map_[command_name];
   }
   return current_command;
}

void ScriptCommandFactory::AddCommand(IScriptCommand* action, std::initializer_list<std::string >commands)
{
   action->InitCommand(context_);

   auto it = commands.begin();
   if (it == commands.end())
      return;

   std::vector<std::string> command_list;
   function_map_[*it] = action;
}


////////////////////////////////////////////////////////
///
/// Commands
///
////////////////////////////////////////////////////////


////////////////////////////////////////////////////////
/// csl_version
///
bool CommandCslVersion::Execute(std::vector<std::string>& param)
{
   if (param.size() < 2)
   {
      return true;
   }

   context_->SetVersion(param[1]);

   return true;
}

////////////////////////////////////////////////////////
/// reset <soft | hard (default)>
///
bool CommandReset::Execute(std::vector<std::string>& param)
{
   // Reset type
   unsigned int type_reset = 0;
   if (param.size() > 2)
   {
      if (param[1] == "soft") type_reset = 1;
   }

   context_->GetEmulation()->GetEngine()->OnOff();

   if (type_reset == 0)
   {
      // Reset all RAM
      // TODO
   }

   return true;
}