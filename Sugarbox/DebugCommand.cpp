
#include "DebugCommand.h"
#include <qlogging.h>

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
