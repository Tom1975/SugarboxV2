
#include "GdbDebugWorker.h"

#include <QtWebSockets/QtWebSockets>


#define STATE_DEFAULT      ""
#define STATE_CPU_STEP     "cpu-step"

//////////////////////////////////////////////
// callback & signals
GdbDebugWorker::GdbDebugWorker(QTcpSocket *socket, int socketDescriptor, Emulation* emulation) : socket_(socket), socketDescriptor_(socketDescriptor), emulation_(emulation)
{
   prompt_ = "";
   state_ = STATE_NONE;
}

void GdbDebugWorker::EnterCpuStep()
{
   prompt_ = STATE_CPU_STEP;
   state_ = STATE_STEP;
}

void GdbDebugWorker::ExitCpuStep()
{
   prompt_ = "";
   state_ = STATE_NONE;
}

void GdbDebugWorker::WritePrompt()
{
   socket_->write("command");
   if (prompt_.size() > 0)
   {
      socket_->write("@");
      socket_->write(prompt_.c_str());
   }
   socket_->write("> ");

}

void GdbDebugWorker::Break(unsigned int nb_opcodes)
{
   // Done. Send ... something : todo
   char out[64];
   sprintf(out, "Returning after %d opcodes\n", nb_opcodes);
   qDebug() << out;
   socket_->write(out);
   EnterCpuStep();
   WritePrompt();
}

void GdbDebugWorker::BreakpointReached(IBreakpointItem* breakpoint)
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
 