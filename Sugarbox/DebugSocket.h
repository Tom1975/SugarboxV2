#pragma once

#include <deque>
#include <functional>

#include <QtWebSockets/QtWebSockets>
#include <QTcpServer>
#include <QTcpSocket>
#include "Emulation.h"

class DebugSocket : public QTcpServer
{
   Q_OBJECT
public:
   explicit DebugSocket(QObject* parent, Emulation* emulation);
   void StartServer();

signals:

public slots:

protected:
   void incomingConnection(int socketDescriptor);
protected:
   Emulation* emulation_;

};

class DebugThread : public QThread, public IBeakpointNotifier
{
   Q_OBJECT
public:
   explicit DebugThread(Emulation* emulation, int iID, QObject *parent = 0);
   
   void run();
   virtual void NotifyBreak(unsigned int nb_opcodes);
   virtual void BreakpointEncountered(IBreakpointItem* breakpoint);

signals:
   void Error(QTcpSocket::SocketError socketerror);
   void SignalBreakpoint(IBreakpointItem* breakpoint);
   void SignalBreak(unsigned int nb_opcodes);

public slots:
   void ReadyRead();
   void Disconnected();

protected:
   Emulation* emulation_;

   // Socket handling
   QTcpSocket *socket_;
   int socketDescriptor_;
   std::string pending_command_;
   

   // State machine
   enum {
      STATE_NONE,
      STATE_STEP
   } state_;
   std::string prompt_;

   // Command list
   typedef struct RemoteCommand
   {
      std::function<bool(std::deque<std::string>&)> execute;
      std::string help;
   };
   std::map<std::string, RemoteCommand > function_map_;
   std::function<bool(std::deque<std::string>&)> current_command_;
   void InitMap();

   // Debug commands
   bool About(std::deque<std::string>);
   bool ClearBreakpoints(std::deque<std::string> param);
   bool CpuStep(std::deque<std::string> param);
   bool DisableBreakpoint(std::deque<std::string> param);
   bool DisableBreakpoints(std::deque<std::string> param);
   bool Disassemble(std::deque<std::string> param);
   bool EnableBreakpoint(std::deque<std::string> param);
   bool EnableBreakpoints(std::deque<std::string> param);
   bool EnterCpuStep(std::deque<std::string> param);
   bool ExtendedStack(std::deque<std::string>);
   bool GetCurrentMachine(std::deque<std::string>);
   bool GetCpuFrequency(std::deque<std::string>);
   bool GetRegisters(std::deque<std::string>);
   bool GetVersion(std::deque<std::string>);
   bool HardReset(std::deque<std::string> param);
   bool ReadMemory(std::deque<std::string>);
   bool Run(std::deque<std::string> param);
   bool SetBreakpoint(std::deque<std::string> param);
};

class DebugWorker : public QObject
{
   Q_OBJECT
public:
   DebugWorker(QTcpSocket *socket);

public slots:
   void Break(unsigned int nb_opcodes);
   void BreakpointReached(IBreakpointItem* breakpoint);

protected:
   QTcpSocket *socket_;
};
