#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
using socklen_t = int;
#else
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#endif

#include "DebugServer.h"

#include <iostream>
#include <sstream>
#include <cstring>


using json = nlohmann::json;

DebugServer::DebugServer(Emulation* emulation, int port )
   : emulation_(emulation), port_(port)
{
#ifdef _WIN32
   WSADATA wsa;
   WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
}

DebugServer::~DebugServer()
{
   stop();
#ifdef _WIN32
   WSACleanup();
#endif
}

void DebugServer::StartServer()
{
   running_ = true;
   thread_ = std::thread(&DebugServer::serverThread, this);
   thread_send_ = std::thread(&DebugServer::networkThread, this);
}

bool sendAll(int s, const char* buf, size_t len)
{
    size_t sent = 0;
    while (sent < len)
    {
        int n = send(s, buf + sent, int(len - sent), 0);
        if (n <= 0)
            return false;
        sent += n;
    }
    return true;
}

void DebugServer::networkThread()
{
    while (running_)
    {
      if ( clientSocket_ != -1)
      {
         std::string msg = outgoing_queue_.pop();
         std::cout << "Trying to send " << msg << std::endl;
         sendAll(clientSocket_, msg.c_str(), (int)msg.size());
      }
      else
      {
         // sleep
         sleep(1);
      }
    }
}

void DebugServer::stop()
{
   running_ = false;

   if (serverSocket_ != -1)
   {
#ifdef _WIN32
      closesocket(serverSocket_);
#else
      close(serverSocket_);
#endif
      serverSocket_ = -1;
   }

   if (thread_.joinable())
      thread_.join();
   if (thread_send_.joinable())
      thread_send_.join();
}

void DebugServer::serverThread()
{
   serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
   if (serverSocket_ < 0)
   {
      std::cerr << "Socket creation failed\n";
      return;
   }

   sockaddr_in addr{};
   addr.sin_family = AF_INET;
   addr.sin_addr.s_addr = INADDR_ANY;
   addr.sin_port = htons(port_);

   if (bind(serverSocket_, (sockaddr*)&addr, sizeof(addr)) < 0)
   {
      std::cerr << "Bind failed\n";
      return;
   }

   listen(serverSocket_, 1);

   std::cout << "Z80 Debug server listening on port " << port_ << std::endl;

   while (running_)
   {
      sockaddr_in client{};
      socklen_t len = sizeof(client);
      clientSocket_ = accept(serverSocket_, (sockaddr*)&client, &len);
      if (clientSocket_ < 0)
         continue;

      std::cout << "Debugger connected" << std::endl;
      handleClient(clientSocket_);

#ifdef _WIN32
      closesocket(clientSocket_);
#else
      close(clientSocket_);
#endif
      std::cout << "Debugger disconnected" << std::endl;
   }
}

void  DebugServer::NotifyStop(IDebugerStopped::Reason reason)
{
   json j;
   Z80* z80 = emulation_->GetEngine()->GetProc();

   j["type"]  = "event";
   j["event"]  = "stopped";
   json body;
   body["reason"] = "breakpoint";
   body["threadId"] = 1;
   body["allThreadsStopped"] = true;
   j["body"] = body;

   std::cout << "STOP notified : " << j["event"] << std::endl;
   outgoing_queue_.push(j.dump() + "\n");   
}

void DebugServer::handleClient(int clientSocket)
{
   char buffer[4096];

   // Break emulation
   emulation_->Break();

   while (running_)
   {
      std::memset(buffer, 0, sizeof(buffer));
      int received = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
      if (received <= 0)
         break;

      std::string requestStr(buffer);
      requestStr.erase(requestStr.find_last_not_of("\r\n") + 1);

      json request;
      try
      {
         request = json::parse(requestStr);
      }
      catch (...)
      {
         std::cerr << "Invalid JSON received\n";
         continue;
      }

      json response;

      std::string cmd = request.value("cmd", "");

      std::cout << "Command : " << cmd << std::endl;

      if (cmd == "readRegisters")
      {
         Z80* z80 = emulation_->GetEngine()->GetProc();

         response["AF"] = z80->af_.w;
         response["AF'"] = z80->af_p_.w;
         response["BC"] = z80->bc_.w;
         response["BC'"] = z80->bc_p_.w;
         response["DE"] = z80->de_.w;
         response["DE'"] = z80->de_p_.w;
         response["HL"] = z80->hl_.w;
         response["HL'"] = z80->hl_p_.w;
         response["SP"] = z80->sp_;
         response["PC"] = z80->pc_;
         SendResponse(response);
      }
      else if (cmd == "getState")
      {
         Z80* z80 = emulation_->GetEngine()->GetProc();
         json response;

         // To choose which bank is used
         // PC : source:address. 

         response["pc"] = z80->pc_;
         response["sp"] = z80->sp_;

         response["running"] =
            (emulation_->IsRunning()) ? "true" : "false";
         SendResponse(response);
      }
      else if (cmd == "step")
      {
         response = { {"status", "ok"} };
         SendResponse(response);
         emulation_->Step();
      }
      else if (cmd == "halt")
      {
         response = { {"status", "ok"} };
         SendResponse(response);
         emulation_->Break();
      }
      else if (cmd == "continue")
      {
         response = { {"status", "running"} };
         SendResponse(response);
         // TODO
      }
      else if (cmd == "disassemble")
      {
         // [
         // { "address": 7842, "text": "LD A,(HL)" },
         // { "address": 7843, "text": "INC HL" }
         // ]      
         unsigned int pc = request.value("address", 0);
         unsigned int count = request.value("count", 0);
         // to use...maybe later !
         std::string type = request.value("type", "READ");
         std::string bank = request.value("bank", "0");

         json arr = json::array();
         for (int i = 0; i < count; i++) 
         {
            char out_buffer[128];
            memset(out_buffer, 0x20, sizeof(out_buffer));
            int increment = emulation_->Disassemble(pc, out_buffer, 128 - 7);
            arr.push_back({
               { "address", pc },
               { "instruction", out_buffer }
            }); 
            pc += increment;
         }
         response = {
         { "type", "response" },
         { "command", "disassemble" },
         { "instructions",  arr },
         };
         SendResponse(response);
      }
      else
      {
         response = { {"error", "unknown command"} };
         SendResponse(response);
      }
   }
}

void DebugServer::SendResponse(json response)
{      
   std::string out = response.dump() + "\n";
   std::cout << "Send response  : " << out << std::endl;
   outgoing_queue_.push(out);
}
