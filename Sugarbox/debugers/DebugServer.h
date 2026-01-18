#pragma once

#include <string>
#include <thread>
#include <atomic>

#include "Emulation.h"

class DebugServer
{
public:
   DebugServer(Emulation* emulation, int port = 1234 );
   ~DebugServer();

   void StartServer();
   void stop();

private:
   void serverThread();
   void handleClient(int clientSocket);

   int port_;
   std::atomic<bool> running_{ false };
   std::thread thread_;

   SOCKET serverSocket_;

   Emulation* emulation_;
};
