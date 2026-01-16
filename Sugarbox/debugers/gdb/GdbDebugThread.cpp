
#include "GdbDebugThread.h"

#include <sstream>
#include <functional>
#include <QtCore>
#include <QTcpSocket>
#include <cstdio>

#include "GdbCommands.h"



#define STATE_DEFAULT      ""
#define STATE_CPU_STEP     "cpu-step"

QT_USE_NAMESPACE

#if defined (__unix) || (__MORPHOS__) || (__APPLE__)
#define stricmp strcasecmp
#define strnicmp strncasecmp
#endif


GdbDebugThread::GdbDebugThread(Emulation* emulation, int ID, QObject *parent) :
   emulation_(emulation), QThread(parent), state_(WAITING_START)
{
   pending_command_ = STATE_DEFAULT;

   socketDescriptor_ = ID;

   // Breakpoitn handler
   emulation->AddNotifier(this);

   qDebug() << socketDescriptor_ << " GdbDebugThread Constructor -> Starting thread - Thread ID : " << currentThreadId();
}

void GdbDebugThread::run()
{
   // thread starts here
   qDebug() << socketDescriptor_ << " GdbDebugThread -> Starting thread - Thread ID : " << currentThreadId();
   socket_ = new QTcpSocket();
   worker_ = new GdbDebugWorker(socket_, socketDescriptor_, emulation_);
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

void GdbDebugThread::Disconnected()
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

void GdbDebugThread::ReadyRead()
{
   QByteArray Data = socket_->readAll();
   
   // Add command to potential unfinished string
   std::string tmp = (const char*)Data;
   pending_buffer_.append(tmp);


   bool finished = false;
   while (!finished)
   {
      switch (state_)
      {
         case WAITING_START:
            // remove every character until "$"
            while (pending_buffer_.size() > 0 && pending_buffer_[0] != '$')
            {
               pending_buffer_.erase(0, 1);
            }
            if (pending_buffer_.size() > 0 && pending_buffer_[0] == '$')
            {
               qDebug() << socketDescriptor_ << " Start of command";
               pending_buffer_.erase(0, 1);
               pending_command_.clear();
               state_ = IN_PAYLOAD;
            }
            else
            {
               // waiting for something else
               finished = true;
            }
         break;
         case IN_PAYLOAD:
            // Any commands ?
            while (pending_buffer_.size() > 0 && pending_buffer_[0] != '#')
            {
               pending_command_ += pending_buffer_[0];
               pending_buffer_.erase(0, 1);
            }
            if (pending_buffer_.size() > 0 && pending_buffer_[0] == '#')
            {
               qDebug() << socketDescriptor_ << " Command : " << QString::fromStdString(pending_command_) << " - Waiting for checksum";
               pending_buffer_.erase(0, 1);
               checksum_.clear();
               state_ = IN_CHECKSUM;
            }
            else
            {
               // waiting for something else
               finished = true;
            }
         break;
         case IN_CHECKSUM:
         if ( pending_buffer_.size() >= 2)
         {
            // Verify checksum
            checksum_ = pending_buffer_.substr(0, 2);
            pending_buffer_.erase(0, 2);

            qDebug() << socketDescriptor_ << " checksum : " <<  QString::fromStdString(checksum_ );
            // Execute command
            Execute (pending_command_, checksum_);
            state_ = WAITING_START;
         }
         else
         {  
            // waiting for more characters
            finished = true;
         }
         break;
      }
   }
}


void GdbDebugThread::Execute(std::string command, std::string checksum)
{
   qDebug() << socketDescriptor_ << "Execution : command " << QString::fromStdString(command) << " - checksum : " << QString::fromStdString(checksum);

   // Checksum
   unsigned char compute_checksum = 0;
   for (unsigned int index = 0; index < command.size(); index++)
   {
      compute_checksum += command[index];
   }
   char * endPtr;
   unsigned char read_checksum = strtoul( checksum.c_str(), &endPtr, 16 ); 
   if ( read_checksum == compute_checksum)
   {
      qDebug() << socketDescriptor_ << "Checksum ok";
      socket_->write("+");

      // Handle command
      HandleCommand(command);
   }
   else
   {
      qDebug() << socketDescriptor_ << "Checksum error : " << (int)read_checksum << "instead of" << (int)compute_checksum;
      socket_->write("-");
   }
}

void GdbDebugThread::HandleCommand(std::string command)
{
   // Prefixe command :
   unsigned char prefix = command[0];
   std::vector<std::string> vector;
   vector.push_back(command.substr(1));

   if ( command_map_.find(prefix) != command_map_.end())
   {
      qDebug() << socketDescriptor_ << "Execution of " << QString::fromStdString(vector[0]);
      command_map_[prefix]->Execute(vector);
   }
   else
   {
      // Unknown command
      qDebug() << socketDescriptor_ << "Unknown command";
      SendResponse("E01");
   }

}

void GdbDebugThread::AddCommand (IRemoteCommand* action, char command)
{
   action->InitCommand(this, emulation_);
   command_map_[command] = action;
}

void GdbDebugThread::AddCommand (IRemoteCommand* action, std::initializer_list<std::string >commands)
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

void GdbDebugThread::InitMap()
{
   AddCommand (new RemoteCommandQuery, 'q');
   AddCommand (new RemoteCommandV, 'v');
   AddCommand (new RemoteCommandH, 'H');
   AddCommand (new RemoteCommandC, 'c');
   AddCommand (new RemoteCommandAsk, '?');
   AddCommand (new RemoteCommandStack, 'g');
   

   /*AddCommand(new RemoteCommandAbout(), { "about" });
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
   AddCommand(new RemoteCommandSetBreakpoint(), { "set-breakpoint", "sb" });*/



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

void GdbDebugThread::SendMultilineString(std::string str)
{
   std::vector<std::string> string_lines;
   split(str, '\n', std::back_inserter(string_lines));
   for (auto& it: string_lines)
   {
      socket_->write(it.c_str());
      socket_->write(cr_lf_.c_str());
   }
}

bool GdbDebugThread::Help(std::vector<std::string> param)
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

void GdbDebugThread::NotifyBreak(unsigned int nb_opcodes)
{
   qDebug() << "  NotifyBreak - Thread ID : " << currentThreadId();
   emit SignalBreak(nb_opcodes);
}

void GdbDebugThread::BreakpointEncountered(IBreakpointItem* breakpoint)
{
   qDebug() << "  BreakpointEncountered - Thread ID : " << currentThreadId();
   emit SignalBreakpoint(breakpoint);
}

void GdbDebugThread::SendResponse(const char* response)
{
   std::string reply = response;
   unsigned char checksum = 0;
   for (unsigned int index = 0; index < reply.size(); index++)
   {
      checksum += reply[index];
   }
   char hex_checksum[3] = {0};
   sprintf(hex_checksum, "%2.2X", checksum);
   
   std::string complete_reply = "$" + reply + "#" + hex_checksum;

   socket_->write(complete_reply.c_str());
   qDebug() << socketDescriptor_ << "Response sent : " << QString::fromStdString(complete_reply);
}

void GdbDebugThread::SendEoL()
{
   socket_->write(cr_lf_.c_str());
}

void GdbDebugThread::EnterCpuStep()
{
   worker_->EnterCpuStep();
}

void GdbDebugThread::ExitCpuStep()
{
   worker_->ExitCpuStep();
}

void GdbDebugThread::Log(const char* log)
{
   qDebug() << socketDescriptor_ << log;
}

//////////////////////////////////////////////
// Help command
RemoteCommandHelp::RemoteCommandHelp(GdbDebugThread* debug):debug_(debug)
{
   
}

bool RemoteCommandHelp::Execute(std::vector<std::string>& param)
{
   return debug_->Help(param);
}

std::string RemoteCommandHelp::Help()
{
   return "Display the command list";
}


