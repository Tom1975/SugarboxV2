#include "Script.h"

#include "Emulation.h"


////////////////////////////////////////////////////////
/// Script
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
std::map<std::string, Qt::Key> ScriptCommandFactory::Escape_map_ = {};


void ScriptCommandFactory::AddScanCode(std::string key, ScriptContext* context, unsigned int line, unsigned int column)
{
   Escape_map_[key] = static_cast<Qt::Key>(context->GetEmulation()->GetKeyboardHandler()->GetScanCode(line, column));
}

void ScriptCommandFactory::InitFactory(ScriptContext* context)
{
   context_ = context;

   AddScanCode("\\(ESC)", context, 8, 2);
   AddScanCode("\\(TAB)", context, 8, 4);
   AddScanCode("\\(CAP)", context, 8, 6);
   AddScanCode("\\(SHI)", context, 2, 5);
   AddScanCode("\\(CTR)", context, 2, 7);
   AddScanCode("\\(COP)", context, 2, 1);
   AddScanCode("\\(CLR)", context, 2, 0);
   AddScanCode("\\(DEL)", context, 9, 7);
   AddScanCode("\\(RET)", context, 2, 2);
   AddScanCode("\\(ENT)", context, 0, 6);
   AddScanCode("\\(ARL)", context, 1, 0);
   AddScanCode("\\(ARR)", context, 0, 1);
   AddScanCode("\\(ARU)", context, 0, 0);
   AddScanCode("\\(ARD)", context, 0, 2);
   AddScanCode("\\(F0)", context, 1, 7);
   AddScanCode("\\(F1)", context, 1, 5);
   AddScanCode("\\(F2)", context, 1, 6);
   AddScanCode("\\(F3)", context, 0, 5);
   AddScanCode("\\(F4)", context, 2, 4);
   AddScanCode("\\(F5)", context, 1, 4);
   AddScanCode("\\(F6)", context, 0, 4);
   AddScanCode("\\(F7)", context, 1, 2);
   AddScanCode("\\(F8)", context, 1, 3);
   AddScanCode("\\(F9)", context, 0, 3);
   AddScanCode("\\({)", context, 2, 1);
   AddScanCode("\\(})", context, 2, 3);
   AddScanCode("\\(\\)", context, 2, 6);
   AddScanCode("\\(')", context, 5, 1);


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
   AddCommand(new CommandSnapshotName(), { "snapshot_name" });
   AddCommand(new CommandKeyDelay(), { "key_delay" });
   AddCommand(new CommandKeyOutput(), { "key_output" });
   AddCommand(new CommandKeyFromFile(), { "key_from_file" });
   AddCommand(new CommandWait(), { "wait" });
   AddCommand(new CommandWaitDriveOnOff(), { "wait_driveonoff" });
   AddCommand(new CommandWaitVSyncOffOn(), { "wait_vsyncoffon" });
   AddCommand(new CommandScreenshotName(), { "screenshot_name" });
   AddCommand(new CommandScreenshotDir(), { "screenshot_dir" });
   AddCommand(new CommandScreenshot(), { "screenshot" });
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
   
   if (param.size() >= 2)
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
/// disk_insert <drive> <'file with extension'>
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
   // remove quotes
   param[param_name].erase(std::remove(param[param_name].begin(), param[param_name].end(), '"'), param[param_name].end());
   param[param_name].erase(std::remove(param[param_name].begin(), param[param_name].end(), '\''), param[param_name].end());

   p += param[param_name];
   context_->GetEmulation()->GetEngine()->LoadDisk(p.string().c_str(), drive);
   

   return true;
}

////////////////////////////////////////////////////////
/// disk_dir <'disk directory'> 
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
/// tape_insert <'file with extension'>
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
/// tape_dir <'tape directory'> 
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


////////////////////////////////////////////////////////
/// snapshot_name
///
bool CommandSnapshotName::Execute(std::vector<std::string>& param)
{
   if (param.size() < 2)
   {
      return true;
   }

   context_->SetSnapshotName(param[1]);

   return true;
}

////////////////////////////////////////////////////////
/// key_delay 
///
bool CommandKeyDelay::Execute(std::vector<std::string>& param)
{
   if (param.size() < 3)
   {
      return true;
   }

   unsigned int delay1, delay2, delay3;
   char* end;
   delay1 = strtol(param[1].c_str(), &end, 10);
   delay2 = strtol(param[2].c_str(), &end, 10);
   if (param.size() == 4)
   {
      delay3 = strtol(param[3].c_str(), &end, 10);
   }
   else
   {
      delay3 = delay2;
   }

   context_->SetKeyDelay(delay1, delay2, delay3);

   return true;
}

////////////////////////////////////////////////////////
/// Generic text output
int CommandGenericType::GetNextKey(std::string& line, int index, std::vector<unsigned int>& next)
{
   int return_index = -1;
   if ( index < line.size())
   {
      if (strncmp( &line[index],"\\(", 2) == 0)
      {
         auto endseq = std::find(line.begin()+index, line.end(), ')' );
         std::string spec = line.substr(index, endseq - line.begin() + index);

         if (ScriptCommandFactory::Escape_map_.find(spec) != ScriptCommandFactory::Escape_map_.end())
         {
            next.push_back(ScriptCommandFactory::Escape_map_[spec]);
         }
         else
         {
            return return_index;
         }
         return_index = index + spec.size();
      }
      else if (strncmp( &line[index], "\\{", 2) == 0)
      {
         auto endseq = std::find(line.begin() + index, line.end(), '}');
         std::string spec = line.substr(index, endseq - line.begin() + index);
         if (spec.size()>0)
         {
            //todo
            return_index = index + spec.size();
         }
      }
      else
      {
         next.push_back((int)line[index++]&0xFF);
         return_index = index;
      }
   }

   return return_index;
}

void CommandGenericType::TypeLineOfText(std::string& line)
{
   // For each character :
   int index = 0;
   std::vector<unsigned int> next_char;
   index = GetNextKey(line, index, next_char);
   while (index != -1)
   {
      // Press the key
      for (auto& it : next_char)
      {
         if ( (it & 0xFFFFFF00) == 0)
         {
            context_->GetEmulation()->GetEngine()->GetKeyboardHandler()->CharPressed(it);
         }
         else
         {
            context_->GetEmulation()->GetEngine()->GetKeyboardHandler()->SendScanCode(it, true);
         }
         
      }
      
      // wait
      Wait(context_->GetKeyPressDelay());

      // unpress the key
      for (auto& it : next_char)
      {
         if ((it & 0xFFFFFF00) == 0)
         {
            context_->GetEmulation()->GetEngine()->GetKeyboardHandler()->CharReleased(it);
         }
         else
         {
            context_->GetEmulation()->GetEngine()->GetKeyboardHandler()->SendScanCode(it, false);
         }
      }
      
      // wait again
      // wait
      Wait(context_->GetKeyDelay());

      next_char.clear();
      index = GetNextKey(line, index, next_char);
   }
}

void CommandGenericType::Wait(unsigned int nb_us)
{
   while (nb_us > 0)
   {
      unsigned long tick_to_run;
      if (nb_us < 4000 * 10) // 10ms
      {
         tick_to_run = nb_us;
      }
      else
      {
         tick_to_run = 4000 * 10;
      }
      context_->GetEmulation()->GetEngine()->GetMotherboard()->DebugNew(tick_to_run);
      context_->GetEmulation()->Unlock();

      nb_us -= tick_to_run;
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      context_->GetEmulation()->Lock();
   }
}
////////////////////////////////////////////////////////
/// key_output
///
bool CommandKeyOutput::Execute(std::vector<std::string>& param)
{
   if (param.size() < 2)
   {
      return true;
   }

   // Paste text : Extract what's within quotes
   std::string line = param[1];
   line.erase(std::remove(line.begin(), line.end(), '\''), line.end());

   TypeLineOfText(line);

   return true;
}

////////////////////////////////////////////////////////
/// key_from_file
///
bool CommandKeyFromFile::Execute(std::vector<std::string>& param)
{
   if (param.size() < 2)
   {
      return true;
   }

   // todo

   return true;
}

////////////////////////////////////////////////////////
/// wait
///
bool CommandWait::Execute(std::vector<std::string>& param)
{
   if (param.size() < 2)
   {
      return true;
   }

   // convert to number of us

   unsigned int nb_us = strtol(param[1].c_str(), NULL, 10) * 4;

   Wait(nb_us);

   return true;
}

////////////////////////////////////////////////////////
/// wait_driveonoff
///
bool CommandWaitDriveOnOff::Execute(std::vector<std::string>& param)
{
   if (param.size() < 2)
   {
      return true;
   }

   // todo

   return true;
}

////////////////////////////////////////////////////////
/// wait_vsyncoffon
///
bool CommandWaitVSyncOffOn::Execute(std::vector<std::string>& param)
{
   if (param.size() < 2)
   {
      return true;
   }

   // todo

   return true;
}

////////////////////////////////////////////////////////
/// screenshot_name
///
bool CommandScreenshotName::Execute(std::vector<std::string>& param)
{
   if (param.size() < 2)
   {
      return true;
   }

   context_->SetScreenshotName(param[1]);

   return true;
}

////////////////////////////////////////////////////////
/// screenshot_dir
///
bool CommandScreenshotDir::Execute(std::vector<std::string>& param)
{
   if (param.size() < 2)
   {
      return true;
   }

   context_->SetScreenshotDir(param[1]);

   return true;
}

////////////////////////////////////////////////////////
/// screenshot
///
bool CommandScreenshot::Execute(std::vector<std::string>& param)
{
   // todo

   return true;
}

////////////////////////////////////////////////////////
/// snapshot
///
bool CommandSnapshot::Execute(std::vector<std::string>& param)
{
   // todo

   return true;
}

////////////////////////////////////////////////////////
/// csl_load
///
bool CommandCslLoad::Execute(std::vector<std::string>& param)
{
   if (param.size() < 2)
   {
      return true;
   }

   // todo

   return true;
}