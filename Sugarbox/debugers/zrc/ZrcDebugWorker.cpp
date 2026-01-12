
#include "ZrcDebugWorker.h"

#include <QtWebSockets/QtWebSockets>


#define STATE_DEFAULT      ""
#define STATE_CPU_STEP     "cpu-step"

//////////////////////////////////////////////
// callback & signals
ZrcDebugWorker::ZrcDebugWorker(QTcpSocket *socket, int socketDescriptor, Emulation* emulation) : socket_(socket), socketDescriptor_(socketDescriptor), emulation_(emulation)
{
   prompt_ = "";
   state_ = STATE_NONE;
}

void ZrcDebugWorker::EnterCpuStep()
{
   prompt_ = STATE_CPU_STEP;
   state_ = STATE_STEP;
}

void ZrcDebugWorker::ExitCpuStep()
{
   prompt_ = "";
   state_ = STATE_NONE;
}

void ZrcDebugWorker::WritePrompt()
{
   socket_->write("command");
   if (prompt_.size() > 0)
   {
      socket_->write("@");
      socket_->write(prompt_.c_str());
   }
   socket_->write("> ");

}

void ZrcDebugWorker::Break(unsigned int nb_opcodes)
{
   // Done. Send ... something : todo
   char out[64];
   sprintf(out, "Returning after %d opcodes\n", nb_opcodes);
   qDebug() << out;
   socket_->write(out);
   EnterCpuStep();
   WritePrompt();
}

void ZrcDebugWorker::BreakpointReached(IBreakpointItem* breakpoint)
{
   // Done. Send ... something : todo
   if (breakpoint != nullptr)
   {
      char out[64];
      sprintf(out, "Breakpoint fired:%s\n", breakpoint->GetBreakpointFormat().c_str());
      qDebug() << out;
      socket_->write(out);
      WritePrompt();
   }
}
 