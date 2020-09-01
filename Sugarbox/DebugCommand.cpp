
#include "DebugCommand.h"
#include <qlogging.h>

// Cross compilation : use stricmp / strcasecmp depending on windows / linux
#ifndef _WIN32
#define stricmp strcasecmp
#define strnicmp strncasecmp
#endif

#define ABOUT_STRING       "Sugarbox remote command protocol"
#define CURRENT_VERSION    "10.0"
#define CURRENT_MACHINE    "ZX Spectrum+ 128k"

#define STATE_DEFAULT      ""
#define STATE_CPU_STEP     "cpu-step"


////////////////////////////////////////////////////////
/// About
///
bool RemoteCommandAbout::Execute(std::deque<std::string>&)
{
   callback_->SendResponse(ABOUT_STRING);
   return true;
}

std::string RemoteCommandAbout::Help()
{
   return "Shows about message";
}

////////////////////////////////////////////////////////
/// Disassemble
bool RemoteCommandDisassemble::Execute(std::deque<std::string>& param)
{
   if (param.size() > 1)
   {
      char* endstr;
      unsigned short position = strtol(param[1].c_str(), &endstr, 10) & 0xFFFF;
      int nb_line = 1;
      if (param.size() > 2)
      {
         nb_line = strtol(param[2].c_str(), &endstr, 10);
      }

      for (int i = 0; i < nb_line; i++)
      {
         char out_buffer[128];
         memset(out_buffer, 0x20, sizeof(out_buffer));
         unsigned int pos = 0;
         // Format : adress on 7 char (??)
         pos += 7;

         // Disassemble
         position += emulation_->Disassemble(position, &out_buffer[pos], 128 - 7);

         callback_->SendResponse(out_buffer);
         callback_->SendEoL();
      }
   }
   return true;
}

////////////////////////////////////////////////////////
/// EnterCpuStep
bool RemoteCommandCpuStep::Execute(std::deque<std::string>& param)
{
   // Stop emulation
   emulation_->Break();
   callback_->EnterCpuStep();
   return true;
}

////////////////////////////////////////////////////////
/// ExtendedStack
bool RemoteCommandExtendedStack::Execute(std::deque<std::string>& param)
{
   // Depending on the parameters
   if (param[1] == std::string("get"))
   {
      // Get the stack
      if (param.size() > 2)
      {
         char* endstr;
         int stack_size = strtol(param[2].c_str(), &endstr, 10);
         char stack_trace[128];

         for (int i = 0; i < stack_size; i++)
         {
            // Return stack
            std::snprintf(stack_trace, sizeof(stack_trace), "%4.4XH %s%c", emulation_->GetStackShort(i), emulation_->GetStackType(i), 0x0a);
            callback_->SendResponse(stack_trace);
         }
      }
   }
   return true;
}

////////////////////////////////////////////////////////
/// ExtendedStack
bool RemoteCommandClearMembreakpoints::Execute(std::deque<std::string>& param)
{
   emulation_->ClearBreakpoints();
   return true;
}

////////////////////////////////////////////////////////
/// Disable Breakpoint
bool RemoteCommandDisableBreakpoint::Execute(std::deque<std::string>& param)
{
   if (param.size() > 1)
   {
      char* endstr;
      int bp_number = strtol(param[1].c_str(), &endstr, 10);
      if (bp_number > 0)
      {
         emulation_->DisableBreakpoint(bp_number);
         std::string str = "Disable breakpoint " + std::to_string(bp_number);
         callback_->Log(str.c_str());
      }
   }
   return true;
}

////////////////////////////////////////////////////////
/// Disable Breakpoints
bool RemoteCommandDisableBreakpoints::Execute(std::deque<std::string>& param)
{
   emulation_->DisableBreakpoints();
   callback_->Log ("Clear Breakpoints");
   return true;
}

////////////////////////////////////////////////////////
/// Enable breakpoint
bool RemoteCommandEnableBreakpoint::Execute(std::deque<std::string>& param)
{
   // Get breakpoint number
   if (param.size() > 1)
   {
      char* endstr;
      int bp_number = strtol(param[1].c_str(), &endstr, 10);
      if (bp_number > 0)
      {
         emulation_->EnableBreakpoint(bp_number);
         std::string str = "Enable breakpoint " + std::to_string(bp_number);
         callback_->Log(str.c_str());
      }
   }

   return true;
}

////////////////////////////////////////////////////////
/// Enable breakpoints
bool RemoteCommandEnableBreakpoints::Execute(std::deque<std::string>& param)
{
   emulation_->EnableBreakpoints();
   callback_->Log("Enable all breakpoints");
   return true;
}

////////////////////////////////////////////////////////
/// Get CPU Frequency
bool RemoteCommandGetCPUFrequency::Execute(std::deque<std::string>& param)
{
   callback_->SendResponse("4000000");
   return true;
}

////////////////////////////////////////////////////////
/// Get current machine
bool RemoteCommandGetCurrentMachine::Execute(std::deque<std::string>& param)
{
   callback_->SendResponse(CURRENT_MACHINE);
   return true;
}

////////////////////////////////////////////////////////
/// Get registers
bool RemoteCommandGetRegisters::Execute(std::deque<std::string>& param)
{
   std::vector<std::string> reg_list = emulation_->GetZ80Registers();

   bool skip_first_space = true;
   for (auto &it : reg_list)
   {
      if (skip_first_space)
      {
         skip_first_space = false;
      }
      else
      {
         callback_->SendResponse(" ");
      }
      callback_->SendResponse(it.c_str());
   }
   return true;
}

////////////////////////////////////////////////////////
/// Get version
bool RemoteCommandGetVersion::Execute(std::deque<std::string>& param)
{
   callback_->SendResponse(CURRENT_VERSION);
   return true;
}

////////////////////////////////////////////////////////
/// Hard Reset
bool RemoteCommandHardReset::Execute(std::deque<std::string>& param)
{
   emulation_->HardReset();
   callback_->Log("Hard reset CPU");
   return true;
}
////////////////////////////////////////////////////////
/// Read Memory
bool RemoteCommandReadMemory::Execute(std::deque<std::string>& param)
{
   if (param.size() < 3)
      return true;
   unsigned short address = atoi(param[1].c_str());
   unsigned int size = atoi(param[2].c_str());
   unsigned char* buffer = new unsigned char[size];
   emulation_->ReadMemory(address, buffer, size);

   // Write result to socket
   char * out = new char[size * 2 + 1];
   char* pout = out;
   memset(out, 0, size * 2 + 1);
   for (unsigned int i = 0; i < size; i++)
   {
      std::snprintf(pout, size * 2 + 1, "%02X", buffer[i]);
      pout += 2;

   }
   callback_->SendResponse(out);

   delete buffer;
   return true;
}

////////////////////////////////////////////////////////
/// Run
bool RemoteCommandRun::Execute(std::deque<std::string>& param)
{
   unsigned int nb_opcodes_to_run = 0;

   for (int i = 1; i < param.size(); i++)
   {
      // usable commands : 
      if (stricmp(param[i].c_str(), "-update-immediately") == 0)
      {
         // - update-immediately
         // todo
      }
      else if (stricmp(param[i].c_str(), "-verbose") == 0)
      {
         // - verbose
         // todo
      }
      else if (stricmp(param[i].c_str(), "-no-stop-on-data") == 0)
      {
         // -no-stop-on-data
         // todo
      }
      else
      {
         // a number of opcodes to run.
         char *endptr;
         nb_opcodes_to_run = strtoul(param[i].c_str(), &endptr, 10);
      }
   }

   emulation_->Run(nb_opcodes_to_run);

   return false;
}

////////////////////////////////////////////////////////
/// RemoteCommandSetBreakpoint
bool RemoteCommandSetBreakpoint::Execute(std::deque<std::string>& param)
{
   // Indice
   if (param.size() < 3)
   {
      callback_->SendResponse("Error : Indice is mandatory, as well as a minimal address");
      return true;
   }
   char*endstr;
   int indice = strtol(param[1].c_str(), &endstr, 10);
   if (indice == 0 || indice > NB_BP_MAX)
   {
      // todo : format with NB_BP_MAX
      callback_->SendResponse("Error : Indice should be a number between 1 and 100");
      return true;
   }

   // Remove command name
   param.pop_front();
   // Remove indice
   param.pop_front();
   emulation_->CreateBreakpoint(indice, param);

   return true;
}