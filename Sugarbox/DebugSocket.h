#pragma once

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
   std::map<std::string, std::function<void()> > function_map_;
   void InitMap();

   // Debug commands
   void about();
};
