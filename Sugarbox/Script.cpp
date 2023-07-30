
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

void ScriptContext::SetDriveDir(std::filesystem::path path)
{
   drive_dir_ = path;
}

std::filesystem::path ScriptContext::GetDriveDir()
{
   return drive_dir_;
}

void ScriptContext::SetTapeDir(std::filesystem::path path)
{
   tape_dir_ = path;
}

std::filesystem::path ScriptContext::GetTapeDir()
{
   return tape_dir_;
}

void ScriptContext::SetSnapshotDir(std::filesystem::path path)
{
   snapshot_dir_= path;
}

std::filesystem::path ScriptContext::GetSnapshotDir()
{
   return snapshot_dir_;
}

void ScriptContext::SetScreenshotDir(std::filesystem::path path)
{
   screenshot_dir_ = path;
}

std::filesystem::path ScriptContext::GetScreenshotDir()
{
   return screenshot_dir_;
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
   AddCommand(new CommandCrtcSelect(), { "crtc_select" });
   AddCommand(new CommandDiskInsert(), { "disk_insert" });
   AddCommand(new CommandDiskDir(), { "disk_dir" });
   AddCommand(new CommandTapeInsert(), { "tape_insert" });
   AddCommand(new CommandTapeDir(), { "tape_dir" });
   AddCommand(new CommandTapePlay(), { "tape_play" });
   AddCommand(new CommandTapeStop(), { "tape_stop" });
   AddCommand(new CommandTapeRewind(), { "tape_rewind" });
   AddCommand(new CommandSnapshotLoad(), { "snapshot_load" });
   AddCommand(new CommandSnapshotDir(), { "snapshot_dir" });
   AddCommand(new CommandKeyDelay(), { "key_delay" });
   AddCommand(new CommandKeyOutput(), { "key_output" });
   AddCommand(new CommandKeyFromFile(), { "key_from_file" });
   AddCommand(new CommandWait(), { "wait" });
   AddCommand(new CommandWaitDriveOnOff(), { "wait_driveonoff" });
   AddCommand(new CommandWaitVSyncOffOn(), { "wait_vsyncoffon" });
   AddCommand(new CommandScreenshotName(), { "screenshot_name" });
   AddCommand(new CommandScreenshotDir(), { "screenshot_dir" });
   AddCommand(new CommandScreenshot(), { "screenshot" });
   AddCommand(new CommandSnapshotName(), { "snapshot_name" });
   AddCommand(new CommandSnapshot(), { "snapshot" });
   AddCommand(new CommandCslLoad(), { "csl_load" });

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

////////////////////////////////////////////////////////
/// crtc_select
///
bool CommandCrtcSelect::Execute(std::vector<std::string>& param)
{
   
   if (param.size() > 2)
   {
      CRTC::TypeCRTC type_crtc = CRTC::HD6845S;

      if (param[1] == "0") type_crtc = CRTC::HD6845S;
      else if (param[1] == "1") type_crtc = CRTC::UM6845R;
      else if (param[1] == "1A") type_crtc = CRTC::UM6845R;
      else if (param[1] == "1B") type_crtc = CRTC::UM6845R;
      else if (param[1] == "2") type_crtc = CRTC::MC6845;
      else if (param[1] == "3") type_crtc = CRTC::AMS40489;
      else if (param[1] == "4") type_crtc = CRTC::AMS40226;
      else if (param[1] == "PUSSY") type_crtc = CRTC::UM6845R; // ?

      context_->GetEmulation()->GetEngine()->GetCRTC()->DefinirTypeCRTC(type_crtc);
   }
   
   return true;
}

////////////////////////////////////////////////////////
/// disk_insert <drive> <’file with extension’ >
///
bool CommandDiskInsert::Execute(std::vector<std::string>& param)
{
   unsigned int drive = 0;
   unsigned int param_name = 1;
   if (param.size() == 3)
   {
      drive = (param[1] == "0") ? 0 : 1;
      param_name = 2;
   }

   std::filesystem::path p = context_->GetDriveDir();
   p += param[param_name];
   context_->GetEmulation()->GetEngine()->LoadDisk(p.string().c_str(), drive);
   

   return true;
}

////////////////////////////////////////////////////////
/// disk_dir <’disk directory’> 
///
bool CommandDiskDir::Execute(std::vector<std::string>& param)
{
   unsigned int drive = 0;
   unsigned int param_name = 1;
   if (param.size() < 2)
   {
      return true;
   }

   context_->SetDriveDir(param[param_name]);

   return true;
}

////////////////////////////////////////////////////////
/// tape_insert <’file with extension’>
///
bool CommandTapeInsert::Execute(std::vector<std::string>& param)
{
   unsigned int param_name = 1;
   if (param.size() < 2)
   {
      return true;
   }

   std::filesystem::path p = context_->GetTapeDir();
   p += param[1];
   context_->GetEmulation()->GetEngine()->LoadTape(p.string().c_str());

   return true;
}


////////////////////////////////////////////////////////
/// tape_dir <’tape directory’> 
///
bool CommandTapeDir::Execute(std::vector<std::string>& param)
{
   if (param.size() < 2)
   {
      return true;
   }

   context_->SetTapeDir(param[1]);

   return true;
}

////////////////////////////////////////////////////////
/// tape_play 
///
bool CommandTapePlay::Execute(std::vector<std::string>& param)
{
   context_->GetEmulation()->TapePlay();
   return true;
}

////////////////////////////////////////////////////////
/// tape_stop 
///
bool CommandTapeStop::Execute(std::vector<std::string>& param)
{
   context_->GetEmulation()->TapeStop();
   return true;
}

////////////////////////////////////////////////////////
/// tape_rewind 
///
bool CommandTapeRewind::Execute(std::vector<std::string>& param)
{
   context_->GetEmulation()->TapeRewind();
   return true;
}

////////////////////////////////////////////////////////
/// snapshot_load 
///
bool CommandSnapshotLoad::Execute(std::vector<std::string>& param)
{
   if (param.size() < 2)
   {
      return true;
   }
   std::filesystem::path p = context_->GetSnapshotDir();
   p += param[1];
   context_->GetEmulation()->LoadSnapshot(p.string().c_str());
   return true;
}


////////////////////////////////////////////////////////
/// snapshot_dir 
///
bool CommandSnapshotDir::Execute(std::vector<std::string>& param)
{
   if (param.size() < 2)
   {
      return true;
   }

   context_->SetSnapshotDir(param[1]);

   return true;
}
