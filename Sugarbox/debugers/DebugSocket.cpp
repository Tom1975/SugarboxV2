
#include <sstream>
#include <functional>
#include <QtCore>
#include <QTcpSocket>
#include <cstdio>

#include "DebugSocket.h"


QT_USE_NAMESPACE

#if defined (__unix) || (__MORPHOS__) || (__APPLE__)
#define stricmp strcasecmp
#define strnicmp strncasecmp
#endif

DebugSocket::DebugSocket(QObject* parent, Emulation* emulation, IThreadCreator* creator, unsigned short port) :emulation_(emulation), QTcpServer(parent), creator_(creator), port_(port)
{
}

void DebugSocket::StartServer()
{
   if (!this->listen(QHostAddress::Any, port_))
   {
      qDebug() << "Could not start server";
   }
   else
   {
      qDebug() << "Listening...";
   }
}

void DebugSocket::incomingConnection(qintptr socketDescriptor)
{
   qDebug() << socketDescriptor << " Connecting...";

   QThread *thread = creator_->CreateThread(emulation_, socketDescriptor, this);

   connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
   thread->start();
}

