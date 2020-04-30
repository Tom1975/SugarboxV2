

#include <sstream>

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
   DebugThread *thread = new DebugThread(emulation_, socketDescriptor, this);
   connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
   thread->start();
}


DebugThread::DebugThread(Emulation* emulation, int ID, QObject *parent) :
   emulation_(emulation), QThread(parent)
{
   pending_command_ = "";
   this->socketDescriptor_ = ID;

   // populate command map if necessary
   InitMap();
}

void DebugThread::run()
{
   // thread starts here
   qDebug() << socketDescriptor_ << " Starting thread";
   socket_ = new QTcpSocket();
   if (!socket_->setSocketDescriptor(this->socketDescriptor_))
   {
      emit Error(socket_->error());
      return;
   }

   connect(socket_, SIGNAL(readyRead()), this, SLOT(ReadyRead()), Qt::DirectConnection);
   connect(socket_, SIGNAL(disconnected()), this, SLOT(Disconnected()), Qt::DirectConnection);

   qDebug() << socketDescriptor_ << " Client connected";

   socket_->write("Welcome to ZEsarUX remote command protocol (ZRCP)\nWrite help for available commands\n");
   socket_->write("\ncommand> ");
   // make this thread a loop
   exec();
}

void DebugThread::Disconnected()
{
   qDebug() << socketDescriptor_ << " Disconnected";
   socket_->deleteLater();
   exit(0);
}

template <typename Out>
void split(const std::string &s, char delim, Out result) {
   std::istringstream iss(s);
   std::string item;
   while (std::getline(iss, item, delim)) 
   {
      *result++ = item;
   }
}

void DebugThread::ReadyRead()
{
   QByteArray Data = socket_->readAll();

   qDebug() << socketDescriptor_ << " Data in: " << Data;
   /*
   // Add command to potential unfinished string
   std::string tmp = Data.toStdString();
   pending_command_.append(tmp);
   
   // Only get until the last ';'
   size_t last = pending_command_.find_last_of('\n');
   std::string processed_command = pending_command_.substr(0, last);
   pending_command_ = pending_command_.substr(last + 1);

   // Handle commands from string : Split them
   vector<std::string> command_list;
   split (processed_command, '\n',std::back_inserter(command_list));

   for (auto &it:command_list)
   {
      if ( function_map_.find(it) != function_map_.end())
      {
         function_map_[it]();
      }
      
   }*/
   

   //function_map_[command]();
}

void DebugThread::InitMap()
{
   function_map_["about"] = std::bind(&DebugThread::about, this);
}

void DebugThread::about()
{
}