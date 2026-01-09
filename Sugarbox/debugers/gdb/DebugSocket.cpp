
#include <sstream>
#include <functional>
#include <QtCore>
#include <QTcpSocket>
#include <cstdio>

#include "DebugSocket.h"
#include "DebugThread.h"


#define STATE_DEFAULT      ""
#define STATE_CPU_STEP     "cpu-step"

QT_USE_NAMESPACE

#if defined (__unix) || (__MORPHOS__) || (__APPLE__)
#define stricmp strcasecmp
#define strnicmp strncasecmp
#endif

DebugSocket::DebugSocket(QObject* parent, Emulation* emulation, unsigned short port) :emulation_(emulation), QTcpServer(parent), port_(port)
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
   DebugThread *thread = new DebugThread(emulation_, socketDescriptor, this);
   connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
   thread->start();
}

