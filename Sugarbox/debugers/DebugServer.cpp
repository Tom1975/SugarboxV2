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
         std::cout << "waiting for something to send...";
         std::string msg = outgoing_queue_.pop();
         std::cout << "Trying to send " << msg;
         sendAll(clientSocket_, msg.c_str(), (int)msg.size());
         std::cout << "Done !";
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

   std::cout << "Z80 Debug server listening on port " << port_ << "\n";

   while (running_)
   {
      sockaddr_in client{};
      socklen_t len = sizeof(client);
      clientSocket_ = accept(serverSocket_, (sockaddr*)&client, &len);
      if (clientSocket_ < 0)
         continue;

      std::cout << "Debugger connected\n";
      handleClient(clientSocket_);

#ifdef _WIN32
      closesocket(clientSocket_);
#else
      close(clientSocket_);
#endif
      std::cout << "Debugger disconnected\n";
   }
}

void  DebugServer::NotifyStop()
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

   outgoing_queue_.push(j.dump() + "\n");   
}

void DebugServer::handleClient(int clientSocket)
{
   char buffer[4096];

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

      std::cout << "Request frame : " << cmd;

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
         json response;
         response["state"] =
            (emulation_->IsRunning()) ? "running" : "stopped";
         SendResponse(response);
      }
      else if (cmd == "step")
      {
         response = { {"status", "ok"} };
         SendResponse(response);
         emulation_->Step();
      }
      else if (cmd == "continue")
      {
         response = { {"status", "running"} };
         SendResponse(response);
         // TODO
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
   std::cout << "Send response  : " << out;
   outgoing_queue_.push(out);
}
