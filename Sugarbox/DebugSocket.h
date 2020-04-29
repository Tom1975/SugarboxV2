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

class MyThread : public QThread
{
   Q_OBJECT
public:
   explicit MyThread(int iID, QObject *parent = 0);
   void run();

signals:
   void error(QTcpSocket::SocketError socketerror);

public slots:
   void readyRead();
   void disconnected();

public slots:

private:
   QTcpSocket *socket;
   int socketDescriptor;

};
