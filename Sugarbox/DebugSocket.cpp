

#include <QtCore>
#include <QTcpSocket>
#include <cstdio>

#include "DebugSocket.h"

using namespace std;

QT_USE_NAMESPACE


DebugSocket::DebugSocket(QObject* parent, Emulation* emulation) :emulation_(emulation), QTcpServer(parent)
   
{
}

void DebugSocket::StartServer()
{
   if (!this->listen(QHostAddress::Any, 10000))
   {
      qDebug() << "Could not start server";
   }
   else
   {
      qDebug() << "Listening...";
   }
}

void DebugSocket::incomingConnection(int socketDescriptor)
{
   qDebug() << socketDescriptor << " Connecting...";
   MyThread *thread = new MyThread(socketDescriptor, this);
   connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
   thread->start();
}


MyThread::MyThread(int ID, QObject *parent) :
   QThread(parent)
{
   this->socketDescriptor = ID;
}

void MyThread::run()
{
   // thread starts here
   qDebug() << socketDescriptor << " Starting thread";
   socket = new QTcpSocket();
   if (!socket->setSocketDescriptor(this->socketDescriptor))
   {
      emit error(socket->error());
      return;
   }

   connect(socket, SIGNAL(readyRead()), this, SLOT(readyRead()), Qt::DirectConnection);
   connect(socket, SIGNAL(disconnected()), this, SLOT(disconnected()), Qt::DirectConnection);

   qDebug() << socketDescriptor << " Client connected";

   socket->write("Welcome to ZEsarUX remote command protocol (ZRCP)\nWrite help for available commands\n");
   socket->write("\ncommand> ");
   // make this thread a loop
   exec();
}

void MyThread::disconnected()
{
   qDebug() << socketDescriptor << " Disconnected";
   socket->deleteLater();
   exit(0);
}

void MyThread::readyRead()
{
   QByteArray Data = socket->readAll();

   qDebug() << socketDescriptor << " Data in: " << Data;

   // Convert string to command

}
