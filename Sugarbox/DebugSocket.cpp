
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
   AddCommand(new RemoteCommandClearMembreakpoints(), { "clear-membreakpoints" });
   AddCommand(new RemoteCommandCpuStep(), { "cpu-step", "cs" });
   AddCommand(new RemoteCommandDisassemble(), { "disassemble", "d" });
   AddCommand(new RemoteCommandExtendedStack(), { "extended-stack" });

   AddCommand(new RemoteCommandHelp(this), { "help", "?" });

   /*
   function_map_["disable-breakpoint"] = {std::bind(&DebugWorker::DisableBreakpoint, worker_, std::placeholders::_1), "Disable specific breakpoint"};
   function_map_["disable-breakpoints"] = { std::bind(&DebugWorker::DisableBreakpoints, worker_, std::placeholders::_1), "Disable breakpoints" };
   function_map_["enable-breakpoint"] = { std::bind(&DebugWorker::EnableBreakpoint, worker_, std::placeholders::_1), "Enable specific breakpoint" };
   function_map_["enable-breakpoints"] = {std::bind(&DebugWorker::EnableBreakpoints, worker_, std::placeholders::_1), "Enable all breakpoints"};
   function_map_["get-cpu-frequency"] = { std::bind(&DebugWorker::GetCpuFrequency, worker_, std::placeholders::_1), "Get cpu frequency in HZ" };
   function_map_["get-current-machine"] = {std::bind(&DebugWorker::GetCurrentMachine, worker_, std::placeholders::_1), "Returns current machine name"};
   function_map_["get-registers"] = {std::bind(&DebugWorker::GetRegisters, worker_, std::placeholders::_1), "Get CPU registers"};
   function_map_["get-version"] = {std::bind(&DebugWorker::GetVersion, worker_, std::placeholders::_1), "Shows emulator version"};
   function_map_["hard-reset-cpu"] = {std::bind(&DebugWorker::HardReset, worker_, std::placeholders::_1), "Hard resets the machine"};
   function_map_["read-memory"] = {std::bind(&DebugWorker::ReadMemory, worker_, std::placeholders::_1), "Dumps memory at address."};
   function_map_["run"] = {std::bind(&DebugWorker::Run, worker_, std::placeholders::_1), "Run cpu when on cpu step mode. Returns when a breakpoint is fired."};
   function_map_["set-breakpoint"] = { std::bind(&DebugWorker::SetBreakpoint, worker_, std::placeholders::_1), "Sets a breakpoint at desired index entry with condition. If no condition set, breakpoint will be handled as disabled\n" };
   */

   // todo 
   // cpu-code-coverage get
   // cpu-code-coverage clear
   // cpu-history get 0
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

bool DebugWorker::ExtendedStack(std::deque<std::string> param)
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
            std::snprintf(stack_trace, sizeof(stack_trace), "%4.4XH %s%c", emulation_->GetStackShort(i), emulation_->GetStackType(i), 0x0a);
            socket_->write(stack_trace);
         }
      }     
   }
   return true;
}

bool DebugWorker::GetVersion(std::deque<std::string>)
{
   socket_->write(CURRENT_VERSION);
   qDebug() << socketDescriptor_ << CURRENT_VERSION;
   return true;
}

bool DebugWorker::GetCpuFrequency(std::deque<std::string>)
{
   socket_->write("4000000");
   qDebug() << socketDescriptor_ << "4000000";
   return true;
}

bool DebugWorker::GetCurrentMachine(std::deque<std::string>)
{
   socket_->write(CURRENT_MACHINE);
   qDebug() << socketDescriptor_ << CURRENT_MACHINE;
   return true;
}


bool DebugWorker::GetRegisters(std::deque<std::string>)
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
   return true;
}

bool DebugWorker::ReadMemory (std::deque<std::string> param)
{
   if (param.size() < 3)
      return true;
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
      std::snprintf(pout, size*2+1, "%02X", buffer[i]);
      pout += 2;
      
   }
   socket_->write(out);

   delete buffer;
   return true;
}

bool DebugWorker::Run(std::deque<std::string> param)
{
   unsigned int nb_opcodes_to_run = 0;

   for (int i = 1; i < param.size(); i++)
   {
      // usable commands : 
      if (stricmp(param[i].c_str(), "-update-immediately") == 0)
      {
         // - update-immediately
         // todo
      }
      else if (stricmp(param[i].c_str(), "-verbose") == 0)
      {
         // - verbose
         // todo
      }
      else if (stricmp(param[i].c_str(), "-no-stop-on-data") == 0)
      {
         // -no-stop-on-data
         // todo
      }
      else 
      {
         // a number of opcodes to run.
         char *endptr;
         nb_opcodes_to_run = strtoul(param[i].c_str(), &endptr, 10);
      }

   }

   emulation_->Run(nb_opcodes_to_run);

   return false;
}

bool DebugWorker::HardReset(std::deque<std::string> param)
{
   emulation_->HardReset();
   qDebug() << "Hard reset CPU";
   return true;
}

bool DebugWorker::EnableBreakpoint(std::deque<std::string> param)
{
   // Get breakpoint number
   if (param.size() > 1)
   {
      char* endstr;
      int bp_number = strtol(param[1].c_str(), &endstr, 10);
      if (bp_number>0)
      {
         emulation_->EnableBreakpoint(bp_number);
         qDebug() << "Enable BReakpoint " << bp_number;
      }
   }
   
   return true;
}

bool DebugWorker::EnableBreakpoints(std::deque<std::string> param)
{
   emulation_->EnableBreakpoints();
   qDebug() << "Clear Breakpoints";
   return true;
}

bool DebugWorker::DisableBreakpoint(std::deque<std::string> param)
{
   if (param.size() > 1)
   {
      char* endstr;
      int bp_number = strtol(param[1].c_str(), &endstr, 10);
      if (bp_number > 0)
      {
         emulation_->DisableBreakpoint(bp_number);
         qDebug() << "Enable BReakpoint " << bp_number;
      }
   }
   return true;
}

bool DebugWorker::DisableBreakpoints(std::deque<std::string> param)
{
   emulation_->DisableBreakpoints();
   qDebug() << "Clear Breakpoints";
   return true;
}


bool DebugWorker::SetBreakpoint(std::deque<std::string> param)
{
   // Indice
   if (param.size() < 3)
   {
      socket_->write("Error : Indice is mandatory, as well as a minimal address");
      return true;
   }
   char*endstr;
   int indice = strtol(param[1].c_str(), &endstr, 10);
   if (indice == 0 ||indice > NB_BP_MAX)
   {
      // todo : format with NB_BP_MAX
      socket_->write("Error : Indice should be a number between 1 and 100");
      return true;
   }

   // Remove command name
   param.pop_front();
   // Remove indice
   param.pop_front();
   emulation_->CreateBreakpoint(indice, param);

   return true;
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
   WritePrompt();
}

void DebugWorker::BreakpointReached(IBreakpointItem* breakpoint)
{
   // Done. Send ... something : todo
   char out[64];
   sprintf(out, "Breakpoint fired:%s\n", breakpoint->GetBreakpointFormat().c_str());
   qDebug() << out;
   socket_->write(out);
   WritePrompt();
}
 