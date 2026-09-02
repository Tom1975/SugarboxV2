
#include <sstream>
#include <functional>
#include <QtCore>
#include <QTcpSocket>
#include <cstdio>

#include "ZrcDebugThread.h"

#define STATE_DEFAULT      ""
#define STATE_CPU_STEP     "cpu-step"

QT_USE_NAMESPACE

#if defined (__unix) || (__MORPHOS__) || (__APPLE__)
#define stricmp strcasecmp
#define strnicmp strncasecmp
#endif


ZrcDebugThread::ZrcDebugThread(Emulation* emulation, int ID, QObject *parent) :
   emulation_(emulation), QThread(parent)
{
   pending_command_ = STATE_DEFAULT;

   socketDescriptor_ = ID;

   // Breakpoitn handler
   emulation->AddNotifier(this);

   qDebug() << socketDescriptor_ << " ZrcDebugThread Constructor -> Starting thread - Thread ID : " << currentThreadId();
}

void ZrcDebugThread::run()
{
   // thread starts here
   qDebug() << socketDescriptor_ << " ZrcDebugThread -> Starting thread - Thread ID : " << currentThreadId();
   socket_ = new QTcpSocket();
   worker_ = new ZrcDebugWorker(socket_, socketDescriptor_, emulation_);
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

   // make this thread a loop
   exec();
}

void ZrcDebugThread::Disconnected()
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

void ZrcDebugThread::ReadyRead()
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

            std::vector<std::string> command_parameters;
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

void ZrcDebugThread::AddCommand (IRemoteCommand* action, std::initializer_list<std::string >commands)
{
   action->InitCommand(this, emulation_);

   auto it = commands.begin();
   if (it == commands.end())
      return;

   std::vector<std::string> command_list;
   function_map_[*it] = action;
   command_list.push_back(*it);
   while (++it != commands.end())
   {
      alternate_command_[*it] = action;
      command_list.push_back(*it);
   }
   command_list_[action] = command_list;
}

void ZrcDebugThread::InitMap()
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
   AddCommand(new RemoteCommandReadMemory(), { "read-memory" });
   AddCommand(new RemoteCommandRun(), { "run", "r" });
   AddCommand(new RemoteCommandSetBreakpoint(), { "set-breakpoint", "sb" });

   AddCommand(new ZrcRemoteCommandHelp(this), { "help", "?" });
   

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

void ZrcDebugThread::SendMultilineString(std::string str)
{
   std::vector<std::string> string_lines;
   split(str, '\n', std::back_inserter(string_lines));
   for (auto& it: string_lines)
   {
      socket_->write(it.c_str());
      socket_->write(cr_lf_.c_str());
   }
}

bool ZrcDebugThread::Help(std::vector<std::string> param)
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

void ZrcDebugThread::NotifyBreak(unsigned int nb_opcodes)
{
   qDebug() << "  NotifyBreak - Thread ID : " << currentThreadId();
   emit SignalBreak(nb_opcodes);
}

void ZrcDebugThread::BreakpointEncountered(IBreakpointItem* breakpoint)
{
   qDebug() << "  BreakpointEncountered - Thread ID : " << currentThreadId();
   emit SignalBreakpoint(breakpoint);
}

void ZrcDebugThread::SendResponse(const char* response)
{
   socket_->write(response);
   qDebug() << socketDescriptor_ << response;
}
void ZrcDebugThread::SendEoL()
{
   socket_->write(cr_lf_.c_str());
}

void ZrcDebugThread::EnterCpuStep()
{
   worker_->EnterCpuStep();
}

void ZrcDebugThread::ExitCpuStep()
{
   worker_->ExitCpuStep();
}

void ZrcDebugThread::Log(const char* log)
{
   qDebug() << socketDescriptor_ << log;
}

//////////////////////////////////////////////
// Help command
ZrcRemoteCommandHelp::ZrcRemoteCommandHelp(ZrcDebugThread* debug):debug_(debug)
{
   
}

bool ZrcRemoteCommandHelp::Execute(std::vector<std::string>& param)
{
   return debug_->Help(param);
}

std::string ZrcRemoteCommandHelp::Help()
{
   return "Display the command list";
}


