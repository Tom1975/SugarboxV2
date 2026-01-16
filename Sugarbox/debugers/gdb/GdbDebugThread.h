#pragma once

#include <functional>

#include <QtWebSockets/QtWebSockets>
#include <QTcpSocket>
#include "Emulation.h"

#include "../DebugCommand.h"

#include "GdbDebugWorker.h"

class GdbDebugThread : public QThread, public IBeakpointNotifier, public ICommandResponse
{
   Q_OBJECT
public:
   explicit GdbDebugThread(Emulation* emulation, int iID, QObject *parent = 0);
   
   void run();
   virtual void NotifyBreak(unsigned int nb_opcodes);
   virtual void BreakpointEncountered(IBreakpointItem* breakpoint);

   void SendResponse(const char* response);
   void SendEoL();
   void EnterCpuStep();
   void ExitCpuStep();
   void Log(const char*);
   bool Help(std::vector<std::string> param);
   

signals:
   void Error(QTcpSocket::SocketError socketerror);
   void SignalBreakpoint(IBreakpointItem* breakpoint);
   void SignalBreak(unsigned int nb_opcodes);

public slots:
   void ReadyRead();
   void Disconnected();

protected:
   
   void Execute(std::string command, std::string checksum);
   void HandleCommand(std::string command);
   
   void AddCommand (IRemoteCommand* action, char command);
   void AddCommand(IRemoteCommand* action, std::initializer_list<std::string >commands);
   void SendMultilineString(std::string str);

   Emulation* emulation_;

   // Socket handling
   GdbDebugWorker * worker_;
   QTcpSocket *socket_;
   int socketDescriptor_;
   std::string pending_buffer_;
   std::string cr_lf_;
   std::string pending_command_;
   std::string checksum_;

   std::map<std::string, IRemoteCommand* > function_map_;
   std::map<std::string, IRemoteCommand* > alternate_command_;
   std::map<IRemoteCommand*, std::vector<std::string>> command_list_;

   std::map<char, IRemoteCommand* > command_map_;

   IRemoteCommand* current_command_;
   void InitMap();

   enum {
      WAITING_START,
      IN_PAYLOAD,
      IN_CHECKSUM
   } state_;
};


class RemoteCommandHelp : public IRemoteCommand
{
public:
   RemoteCommandHelp(GdbDebugThread* debug);
   virtual bool Execute(std::vector<std::string>&);
   virtual std::string Help();
protected:
   GdbDebugThread* debug_;
};