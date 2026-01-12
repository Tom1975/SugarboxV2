#pragma once

#include <functional>

#include <QtWebSockets/QtWebSockets>
#include <QTcpSocket>
#include "Emulation.h"

#include "../DebugCommand.h"

#include "ZrcDebugWorker.h"

class ZrcDebugThread : public QThread, public IBeakpointNotifier, public ICommandResponse
{
   Q_OBJECT
public:
   explicit ZrcDebugThread(Emulation* emulation, int iID, QObject *parent = 0);
   
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
   void AddCommand(IRemoteCommand* action, std::initializer_list<std::string >commands);
   void SendMultilineString(std::string str);

   Emulation* emulation_;

   // Socket handling
   ZrcDebugWorker * worker_;
   QTcpSocket *socket_;
   int socketDescriptor_;
   std::string pending_command_;
   std::string cr_lf_;

   std::map<std::string, IRemoteCommand* > function_map_;
   std::map<std::string, IRemoteCommand* > alternate_command_;
   std::map<IRemoteCommand*, std::vector<std::string>> command_list_;

   IRemoteCommand* current_command_;
   void InitMap();

};


class ZrcRemoteCommandHelp : public IRemoteCommand
{
public:
   ZrcRemoteCommandHelp(ZrcDebugThread* debug);
   virtual bool Execute(std::vector<std::string>&);
   virtual std::string Help();
protected:
   ZrcDebugThread* debug_;
};