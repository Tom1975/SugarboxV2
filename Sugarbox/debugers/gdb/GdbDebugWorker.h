#pragma once

#include <QTcpServer>

#include "Emulation.h"

class GdbDebugWorker : public QObject
{
   Q_OBJECT
public:
   GdbDebugWorker(QTcpSocket *socket, int socketDescriptor, Emulation* emulation);

   void WritePrompt();

public slots:
   void Break(unsigned int nb_opcodes);
   void BreakpointReached(IBreakpointItem* breakpoint);

public:
   // Debug commands
   void EnterCpuStep();
   void ExitCpuStep();

protected:
   // State machine
   enum {
      STATE_NONE,
      STATE_STEP
   } state_;
   std::string prompt_;

   QTcpSocket *socket_;
   int socketDescriptor_;

   Emulation* emulation_;
};
