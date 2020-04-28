

#include <QtCore>
#include <QTcpSocket>
#include <cstdio>

#include "DebugSocket.h"

using namespace std;

QT_USE_NAMESPACE

static QString getIdentifier(QWebSocket *peer)
{
   return QStringLiteral("%1:%2").arg(peer->peerAddress().toString(),
      QString::number(peer->peerPort()));
}

DebugSocket::DebugSocket(QObject* parent, Emulation* emulation):emulation_(emulation), QObject(parent)
{
   quint16 port = 10000;

   server = new QTcpServer(this);

   connect(server, SIGNAL(newConnection()), this, SLOT(newConnection()));

   if (!server->listen(QHostAddress::Any, port))
   {
      qDebug() << "Server could not start!";
   }
   else
   {
      qDebug() << "Server started!";
   }
}

DebugSocket::~DebugSocket()
{
}


//! [onNewConnection]
void DebugSocket::newConnection()
{
   QTcpSocket *socket = server->nextPendingConnection();

   socket->write("Client connected\r\n");
   socket->flush();

   socket->waitForBytesWritten(3000);

   socket->close();
}
//! [onNewConnection]
