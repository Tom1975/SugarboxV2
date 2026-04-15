////////////////////////////////////////////////////////
// 
//  DAP Class adapter : This class will communicate with DAP 
// to provide a way to debug z80 application through Sugarbox
// 
#pragma once

#include <string>
#include <thread>
#include <atomic>

#include "json.hpp"

#include "Emulation.h"

#include <queue>
#include <mutex>
#include <condition_variable>

#ifdef _WIN32
   // link with Ws2_32.lib
   #pragma comment(lib,"Ws2_32.lib")

   #include <winsock2.h>
   #include <ws2tcpip.h>
#endif

class DebugMessageQueue
{
public:
    void push(std::string msg)
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_queue.push(std::move(msg));
        }
        m_cv.notify_one();
    }

    std::string pop()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [&]{ return !m_queue.empty(); });

        std::string msg = std::move(m_queue.front());
        m_queue.pop();
        return msg;
    }

private:
    std::queue<std::string> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cv;
};

class DebugServer
{
public:
   DebugServer(Emulation* emulation, int port = 1234 );
   ~DebugServer();

   void StartServer();
   void stop();
   void NotifyStop(IDebugerStopped::Reason reason);

private:
   void serverThread();
   void networkThread();
   void handleClient(int clientSocket);
   void SendResponse(nlohmann::json response);


   void HandleReadMemory(const nlohmann::json& request);
   void HandleGetMemBanks();

   int port_;
   std::atomic<bool> running_{ false };
   std::thread thread_;
   std::thread thread_send_;

#ifdef _WIN32
   SOCKET serverSocket_;
   SOCKET clientSocket_;
#else
   int serverSocket_;
   int clientSocket_;
#endif

   DebugMessageQueue outgoing_queue_;
   Emulation* emulation_;
};
