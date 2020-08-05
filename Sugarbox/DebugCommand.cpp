
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