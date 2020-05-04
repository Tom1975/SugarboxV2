

#include <sstream>
#include <functional>
#include <QtCore>
#include <QTcpSocket>
#include <cstdio>

#include "DebugSocket.h"


#define ABOUT_STRING       "Sugarbox remote command protocol"
#define CURRENT_VERSION    "10.0"
#define CURRENT_MACHINE    "ZX Spectrum+ 128k"

#define STATE_DEFAULT      ""
#define STATE_CPU_STEP     "cpu-step"

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
   pending_command_ = STATE_DEFAULT;
   prompt_ = "";
   state_ = STATE_NONE;

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

   // Handle backspace (to make command line debug easier..)
   for (unsigned int i = 0; i < pending_command_.size(); i++)
   {
      if (pending_command_[i] == 8)
      {
         if (i > 0)
            pending_command_.erase(i - 1, 2);
         else
            pending_command_.erase(0, 1);
      }
   }
   
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
               //socket_->write("bad command\r\n");
               qDebug() << "bad command";
            }

         }
         socket_->write("\n");
      }
      socket_->write("command");
      if (prompt_.size() > 0)
      {
         socket_->write("@");
         socket_->write(prompt_.c_str());
      }
      socket_->write("> ");      
      
   }
}

void DebugThread::InitMap()
{
   function_map_["about"] = std::bind(&DebugThread::About, this, std::placeholders::_1);
   function_map_["clear-membreakpoints"] = std::bind(&DebugThread::ClearBreakpoints, this, std::placeholders::_1);
   function_map_["disassemble"]= std::bind(&DebugThread::Disassemble, this, std::placeholders::_1);
   function_map_["enter-cpu-step"] = std::bind(&DebugThread::EnterCpuStep, this, std::placeholders::_1);
   function_map_["extended-stack"] = std::bind(&DebugThread::ExtendedStack, this, std::placeholders::_1);
   function_map_["get-current-machine"] = std::bind(&DebugThread::GetCurrentMachine, this, std::placeholders::_1);
   function_map_["get-registers"] = std::bind(&DebugThread::GetRegisters, this, std::placeholders::_1);
   function_map_["get-version"] = std::bind(&DebugThread::GetVersion, this, std::placeholders::_1);
   function_map_["hard-reset-cpu"] = std::bind(&DebugThread::HardReset, this, std::placeholders::_1);
   function_map_["read-memory"] = std::bind(&DebugThread::ReadMemory, this, std::placeholders::_1);

   // todo 

   // cpu-step
   // run
   // extended-stack get 100
   // get-cpu-frequency
   // reset-tstates-partial
   // disassemble 3872
   // cpu-code-coverage get
   // get-tstates-partial
   // cpu-code-coverage clear
   // get-cpu-frequency
   // cpu-history get 0
   // quit

}

void DebugThread::About(std::deque<std::string>)
{
   socket_->write(ABOUT_STRING);
   qDebug() << socketDescriptor_ << ABOUT_STRING;
}

void DebugThread::Disassemble(std::deque<std::string> param)
{
   if (param.size() > 1)
   {
      char* endstr;
      int position = strtol(param[1].c_str(), &endstr, 10);
      int nb_line = 1;
      if (param.size() > 2)
      {
         nb_line = strtol(param[2].c_str(), &endstr, 10);
      }

      for (int i = 0; i < nb_line; i++)
      {
         char out_buffer[128];
         memset(out_buffer, 0, sizeof(out_buffer));
         unsigned int pos = 0;
         // Format : adress on 7 char (??)
         pos += 7;

         // Disassemble

      }
   }

}

void DebugThread::EnterCpuStep(std::deque<std::string>)
{
   prompt_ = STATE_CPU_STEP;
   state_ = STATE_STEP;
   // Stop emulation
   emulation_->Break();
}

void DebugThread::ExtendedStack(std::deque<std::string> param)
{
   // Depending on the parameters
   if (param[1] == std::string("get"))
   {
      // Get the stack
      if (param.size() > 2)
      {
         char* endstr;
         int stack_size = strtol(param[2].c_str(), &endstr, 10);
         char stack_trace[128];
         
         for (int i = 0; i < stack_size; i++)
         {
            // Return stack
            sprintf(stack_trace, "%4.4XH %s%c", emulation_->GetStackShort(i), emulation_->GetStackType(i), 0x0a);
            socket_->write(stack_trace);
         }
      }
      
   }
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
   unsigned short address = atoi(param[1].c_str());
   unsigned int size = atoi(param[2].c_str());
   unsigned char* buffer = new unsigned char[size];
   emulation_->ReadMemory(address, buffer, size);

   // Write result to socket
   char * out = new char[size * 2 + 1];
   char* pout = out;
   memset(out, 0, size * 2 + 1);
   for (unsigned int i  = 0; i < size; i++)
   {
      sprintf(pout, "%02X", buffer[i]);
      pout += 2;
      
   }
   socket_->write(out);

   delete buffer;
}

void DebugThread::HardReset(std::deque<std::string> param)
{
   emulation_->HardReset();
   qDebug() << "Hard reset CPU";
}

void DebugThread::ClearBreakpoints(std::deque<std::string> param)
{
   emulation_->ClearBreakpoints();
   qDebug() << "Clear Breakpoints";
}
