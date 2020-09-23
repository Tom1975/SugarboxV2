
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

#if defined (__unix) || (__MORPHOS__) || (__APPLE__)
#define stricmp strcasecmp
#define strnicmp strncasecmp
#endif

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

   socketDescriptor_ = ID;

   // Breakpoitn handler
   emulation->AddNotifier(this);

   qDebug() << socketDescriptor_ << " DebugThread Constructor -> Starting thread - Thread ID : " << currentThreadId();
}

void DebugThread::run()
{
   // thread starts here
   qDebug() << socketDescriptor_ << " DebugThread -> Starting thread - Thread ID : " << currentThreadId();
   socket_ = new QTcpSocket();
   worker_ = new DebugWorker(socket_, socketDescriptor_, emulation_);
   if (!socket_->setSocketDescriptor(this->socketDescriptor_))
   {
      emit Error(socket_->error());
      return;
   }

   // populate command map if necessary
   InitMap();

   connect(socket_, SIGNAL(readyRead()), this, SLOT(ReadyRead()), Qt::DirectConnection);
   connect(socket_, SIGNAL(disconnected()), this, SLOT(Disconnected()), Qt::DirectConnection);
   connect(this, SIGNAL(SignalBreakpoint(IBreakpointItem*)), worker_, SLOT(BreakpointReached(IBreakpointItem*)));
   connect(this, SIGNAL(SignalBreak(unsigned int)), worker_, SLOT(Break(unsigned int)));

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
   emulation_->RemoveNotifier(this);
   emulation_->Run(0);
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
   bool complete_command = true;
   size_t last = pending_command_.find_last_of('\n');
   if (last != std::string::npos)
   {
      std::string processed_command = pending_command_.substr(0, last);
      pending_command_ = pending_command_.substr(last + 1);

      if (processed_command.size() > 0 && processed_command.back() == '\r')
      {
         cr_lf_ = "\r\n";
         processed_command.pop_back();
      }
      else
      {
         cr_lf_ = "\n";
      }

      if (current_command_ != nullptr)
      {
         // do something smart !
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
            current_command_ = nullptr;
            if (function_map_.find(command_parameters[0]) != function_map_.end())
            {
               current_command_ = function_map_[command_parameters[0]];
            }
            else if(alternate_command_.find(command_parameters[0]) != alternate_command_.end())
            {
               current_command_ = alternate_command_[command_parameters[0]];
            }
            if (current_command_ != nullptr)
            {
               //command_parameters.pop_front();
               complete_command  = current_command_->Execute(command_parameters);
               if (complete_command)
               {
                  current_command_ = nullptr;
               }
            }
            else
            {
               socket_->write("bad command");
               qDebug() << "bad command";
            }

         }
         socket_->write(cr_lf_.c_str());
      }
      if (complete_command)
      {
         worker_->WritePrompt();
      }
   }
}

void DebugThread::AddCommand (IRemoteCommand* action, std::initializer_list<std::string >commands)
{
   action->InitCommand(this, emulation_);

   auto it = commands.begin();
   if (it == commands.end())
      return;

   std::deque<std::string> command_list;
   function_map_[*it] = action;
   command_list.push_back(*it);
   while (++it != commands.end())
   {
      alternate_command_[*it] = action;
      command_list.push_back(*it);
   }
   command_list_[action] = command_list;
}

void DebugThread::InitMap()
{
   AddCommand(new RemoteCommandAbout(), { "about" });
   AddCommand(new RemoteCommandBreak(), { "break", "b" });
   AddCommand(new RemoteCommandClearMembreakpoints(), { "clear-membreakpoints" });
   AddCommand(new RemoteCommandCpuStep(), { "cpu-step", "cs" });
   AddCommand(new RemoteCommandDisableBreakpoint(), { "disable-breakpoint", "db" });
   AddCommand(new RemoteCommandDisableBreakpoints(), { "disable-breakpoints" });
   AddCommand(new RemoteCommandDisassemble(), { "disassemble", "d" });
   AddCommand(new RemoteCommandEnableBreakpoint(), { "enable-breakpoint", "eb" });
   AddCommand(new RemoteCommandEnableBreakpoints(), { "enable-breakpoints"});
   AddCommand(new RemoteCommandExtendedStack(), { "extended-stack" });
   AddCommand(new RemoteCommandGetCPUFrequency(), { "get-cpu-frequency" });
   AddCommand(new RemoteCommandGetCurrentMachine(), { "get-current-machine", "gcm" });
   AddCommand(new RemoteCommandGetRegisters(), { "get-registers", "gr" });
   AddCommand(new RemoteCommandGetVersion(), { "get-version" });
   AddCommand(new RemoteCommandHardReset(), { "hard-reset-cpu" });
   AddCommand(new RemoteCommandHelp(this), { "help", "?" });
   AddCommand(new RemoteCommandReadMemory(), { "read-memory" });
   AddCommand(new RemoteCommandRun(), { "run", "r" });
   AddCommand(new RemoteCommandSetBreakpoint(), { "set-breakpoint", "sb" });

   // todo 
   // cpu-code-coverage get
   // cpu-code-coverage clear
   // cpu-history get 0
   //  extended stack => ExtendedStack
   // get-tstates-partial
   // reset-tstates-partial
   // set-register
   // load-binary
   // smartload
   // sprites
   // get-memory-pages
   // quit

}

void DebugThread::SendMultilineString(std::string str)
{
   std::vector<std::string> string_lines;
   split(str, '\n', std::back_inserter(string_lines));
   for (auto& it: string_lines)
   {
      socket_->write(it.c_str());
      socket_->write(cr_lf_.c_str());
   }
}

bool DebugThread::Help(std::deque<std::string> param)
{
   std::string output = "";

   if (param.size() == 1)
   {
      output = "Available commands:"+ cr_lf_;
      for (auto& it : command_list_)
      {
         bool first_cmd = true;
         for (auto& alt_cmd: it.second)
         {
            if (!first_cmd)
            {
               output += ", ";
            }
            else
            {
               first_cmd = false;
            }
            output += alt_cmd;
         }
         output += cr_lf_;
      }
   }
   else
   {
      if (function_map_.find(param[1]) != function_map_.end())
      {
         output = param[1];

         output += ": ";
         output += function_map_[param[1]]->Help();
         output += cr_lf_;
      }
   }

   qDebug() << "Help : " << cr_lf_.c_str() << output.c_str();
   // Write complex string
   SendMultilineString(output);
   
   return true;

}

void DebugThread::NotifyBreak(unsigned int nb_opcodes)
{
   qDebug() << "  NotifyBreak - Thread ID : " << currentThreadId();
   emit SignalBreak(nb_opcodes);
}

void DebugThread::BreakpointEncountered(IBreakpointItem* breakpoint)
{
   qDebug() << "  BreakpointEncountered - Thread ID : " << currentThreadId();
   emit SignalBreakpoint(breakpoint);
}

void DebugThread::SendResponse(const char* response)
{
   socket_->write(response);
   qDebug() << socketDescriptor_ << response;
}
void DebugThread::SendEoL()
{
   socket_->write(cr_lf_.c_str());
}

void DebugThread::EnterCpuStep()
{
   worker_->EnterCpuStep();
}

void DebugThread::ExitCpuStep()
{
   worker_->ExitCpuStep();
}

void DebugThread::Log(const char* log)
{
   qDebug() << socketDescriptor_ << log;
}

//////////////////////////////////////////////
// Help command
RemoteCommandHelp::RemoteCommandHelp(DebugThread* debug):debug_(debug)
{
   
}

bool RemoteCommandHelp::Execute(std::deque<std::string>& param)
{
   return debug_->Help(param);
}

std::string RemoteCommandHelp::Help()
{
   return "Display the command list";
}


//////////////////////////////////////////////
// callback & signals
DebugWorker::DebugWorker(QTcpSocket *socket, int socketDescriptor, Emulation* emulation) : socket_(socket), socketDescriptor_(socketDescriptor), emulation_(emulation)
{
   prompt_ = "";
   state_ = STATE_NONE;
}

void DebugWorker::EnterCpuStep()
{
   prompt_ = STATE_CPU_STEP;
   state_ = STATE_STEP;
}

void DebugWorker::ExitCpuStep()
{
   prompt_ = "";
   state_ = STATE_NONE;
}

void DebugWorker::WritePrompt()
{
   socket_->write("command");
   if (prompt_.size() > 0)
   {
      socket_->write("@");
      socket_->write(prompt_.c_str());
   }
   socket_->write("> ");

}

void DebugWorker::Break(unsigned int nb_opcodes)
{
   // Done. Send ... something : todo
   char out[64];
   sprintf(out, "Returning after %d opcodes\n", nb_opcodes);
   qDebug() << out;
   socket_->write(out);
   EnterCpuStep();
   WritePrompt();
}

void DebugWorker::BreakpointReached(IBreakpointItem* breakpoint)
{
   // Done. Send ... something : todo
   if (breakpoint != nullptr)
   {
      char out[64];
      sprintf(out, "Breakpoint fired:%s\n", breakpoint->GetBreakpointFormat().c_str());
      qDebug() << out;
      socket_->write(out);
      WritePrompt();
   }
}
 