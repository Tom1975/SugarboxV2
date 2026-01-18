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
#include "json.hpp"

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
}

void DebugServer::stop()
{
   running_ = false;

   if (serverSocket_ != -1)
   {
#ifdef _WIN32
      closesocket(serverSocket_);
#else
      close(m_serverSocket);
#endif
      serverSocket_ = -1;
   }

   if (thread_.joinable())
      thread_.join();
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
      int clientSocket = accept(serverSocket_, (sockaddr*)&client, &len);
      if (clientSocket < 0)
         continue;

      std::cout << "Debugger connected\n";
      handleClient(clientSocket);

#ifdef _WIN32
      closesocket(clientSocket);
#else
      close(clientSocket);
#endif
      std::cout << "Debugger disconnected\n";
   }
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

      if (cmd == "readRegisters")
      {
         // TODO: brancher sur votre CPU Z80
         response = {
             {"AF", 0x1234},
             {"BC", 0x5678},
             {"DE", 0x9ABC},
             {"HL", 0xDEF0},
             {"SP", 0xFFFF},
             {"PC", 0x8000}
         };
      }
      else if (cmd == "step")
      {
         // cpu.step();
         response = { {"status", "ok"} };
      }
      else if (cmd == "continue")
      {
         response = { {"status", "running"} };
      }
      else
      {
         response = { {"error", "unknown command"} };
      }

      std::string out = response.dump() + "\n";
      send(clientSocket, out.c_str(), (int)out.size(), 0);
   }
}
