#pragma once

#include <deque>

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

class DebugThread : public QThread
{
   Q_OBJECT
public:
   explicit DebugThread(Emulation* emulation, int iID, QObject *parent = 0);
   void run();
   
signals:
   void Error(QTcpSocket::SocketError socketerror);

public slots:
   void ReadyRead();
   void Disconnected();

protected:
   Emulation* emulation_;

   // Socket handling
   QTcpSocket *socket_;
   int socketDescriptor_;
   std::string pending_command_;

   // Command list
   std::map<std::string, std::function<void(std::deque<std::string>&)> > function_map_;
   void InitMap();

   // Debug commands
   void About(std::deque<std::string>);
   void GetCurrentMachine(std::deque<std::string>);
   void GetRegisters(std::deque<std::string>);
   void GetVersion(std::deque<std::string>);
   void HardReset(std::deque<std::string> param);
   void ReadMemory(std::deque<std::string>);
};
