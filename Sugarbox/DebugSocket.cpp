

#include <sstream>
#include <functional>
#include <QtCore>
#include <QTcpSocket>
#include <cstdio>

#include "DebugSocket.h"


#define ABOUT_STRING       "Sugarbox remote command protocol"
#define CURRENT_VERSION    "10.0"
#define CURRENT_MACHINE    "ZX Spectrum+ 128k"

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
   
   // Add command to potential unfinished string
   std::string tmp = (const char*)Data;
   pending_command_.append(tmp);
   
   // Only get until the last ';'
   size_t last = pending_command_.find_last_of('\n');
   if (last != std::string::npos)
   {
      std::string processed_command = pending_command_.substr(0, last);
      pending_command_ = pending_command_.substr(last + 1);

      if (processed_command.size() > 0 && processed_command.back() == '\r')
      {
         processed_command.pop_back();
      }

      if (processed_command.size() > 0)
      {
         // Handle commands from string : Split them
         std::vector<std::string> command_list;
         split(processed_command, '\n', std::back_inserter(command_list));

         for (auto &it : command_list)
         {
            qDebug() << socketDescriptor_ << " Command : " << it.c_str();

            std::deque<std::string> command_parameters;
            split(processed_command, ' ', std::back_inserter(command_parameters));

            if (function_map_.find(command_parameters[0]) != function_map_.end())
            {
               //command_parameters.pop_front();
               function_map_[command_parameters[0]](command_parameters);
            }
            else
            {
               socket_->write("bad command");
            }

         }
         socket_->write("\r\n");
      }
      socket_->write("command> ");      
   }
}

void DebugThread::InitMap()
{
   function_map_["about"] = std::bind(&DebugThread::About, this, std::placeholders::_1);
   function_map_["get-version"] = std::bind(&DebugThread::GetVersion, this, std::placeholders::_1);
   function_map_["get-current-machine"] = std::bind(&DebugThread::GetCurrentMachine, this, std::placeholders::_1);
   function_map_["get-registers"] = std::bind(&DebugThread::GetRegisters, this, std::placeholders::_1);
   function_map_["read-memory"] = std::bind(&DebugThread::ReadMemory, this, std::placeholders::_1);
   function_map_["hard_reset_cpu"] = std::bind(&DebugThread::HardReset, this, std::placeholders::_1);
   
}

void DebugThread::About(std::deque<std::string>)
{
   socket_->write(ABOUT_STRING);
   qDebug() << socketDescriptor_ << ABOUT_STRING;
}

void DebugThread::GetVersion(std::deque<std::string>)
{
   socket_->write(CURRENT_VERSION);
   qDebug() << socketDescriptor_ << CURRENT_VERSION;
}

void DebugThread::GetCurrentMachine(std::deque<std::string>)
{
   socket_->write(CURRENT_MACHINE);
   qDebug() << socketDescriptor_ << CURRENT_MACHINE;
}


void DebugThread::GetRegisters(std::deque<std::string>)
{
   std::vector<std::string> reg_list = emulation_->GetZ80Registers();

   bool skip_first_space = true;
   for (auto &it : reg_list)
   {
      if (skip_first_space)
      {
         skip_first_space = false;
      }
      else
      {
         socket_->write(" ");
      }
      socket_->write(it.c_str());
      qDebug() << it.c_str() << " ";
   }
}

void DebugThread::ReadMemory (std::deque<std::string> param)
{
   if (param.size() < 3)
      return;
   unsigned short address = strtol(param[1].c_str(), NULL, 16);
   unsigned int size = strtol(param[2].c_str(), NULL, 16);
   unsigned char* buffer = new unsigned char[size];
   emulation_->ReadMemory(address, buffer, size);

   // Write result to socket
   char * out = new char[size * 2 + 1];
   char* pout = out;
   memset(out, 0, size * 2 + 1);
   for (int i  = 0; i < size; i++)
   {
      sprintf(pout, "%02X", buffer[i]);
      pout += 2;
      
   }
   socket_->write(out);
   qDebug() << out;

   delete buffer;
}

void DebugThread::HardReset(std::deque<std::string> param)
{
   emulation_->HardReset();
}