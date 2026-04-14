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

#include <algorithm>
#include <iostream>
#include <sstream>
#include <cstring>
#include <fstream>
#include <vector>

using json = nlohmann::json;

// ─── Base64 decode ────────────────────────────────────────────────────────────
static std::vector<uint8_t> Base64Decode(const std::string& in)
{
    static const std::string chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::vector<uint8_t> out;
    out.reserve(in.size() * 3 / 4);
    int val = 0, bits = -8;
    for (unsigned char c : in) {
        if (c == '=') break;
        auto pos = chars.find(c);
        if (pos == std::string::npos) continue; // skip whitespace / newlines
        val = (val << 6) | (int)pos;
        bits += 6;
        if (bits >= 0) {
            out.push_back((val >> bits) & 0xFF);
            bits -= 8;
        }
    }
    return out;
}

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
         std::this_thread::sleep_for(std::chrono::milliseconds(1));
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
   std::string reasonStr;
   switch (reason)
   {
      case IDebugerStopped::Step:                 reasonStr = "step"; break;
      case IDebugerStopped::Pause:                reasonStr = "pause"; break;
      case IDebugerStopped::Entry:                reasonStr = "entry"; break;
      case IDebugerStopped::Exception:            reasonStr = "exception"; break;
      case IDebugerStopped::InstructionBreakpoint: reasonStr = "instruction breakpoint"; break;
      case IDebugerStopped::FunctionBreakpoint:   reasonStr = "function breakpoint"; break;
      case IDebugerStopped::DataBreakpoint:       reasonStr = "data breakpoint"; break;
      default:                                    reasonStr = "breakpoint"; break;
   }

   json j;
   j["type"]  = "event";
   j["event"] = "stopped";
   json body;
   body["reason"] = reasonStr;
   body["threadId"] = 1;
   body["allThreadsStopped"] = true;
   j["body"] = body;

   std::cout << "STOP notified : " << reasonStr << std::endl;
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
         response["PC"] = z80->new_instruction_ ? z80->pc_ : z80->GetPC();
         SendResponse(response);
      }
      else if (cmd == "getState")
      {
         Z80* z80 = emulation_->GetEngine()->GetProc();
         json response;

         // To choose which bank is used
         // PC : source:address. 

         // new_instruction_ = true: stopped after full instruction (pc_ = next instr address)
         // new_instruction_ = false: stopped mid-fetch (GetPC() = pc_-1 = current instr address)
         response["pc"] = z80->new_instruction_ ? z80->pc_ : z80->GetPC();
         response["sp"] = z80->sp_;

         response["running"] =
            (emulation_->IsStepping()) ? "true" : "false";
         SendResponse(response);
      }
      else if (cmd == "step")
      {
         response = { {"status", "ok"} };
         SendResponse(response);
         emulation_->StepOver();
      }
      else if (cmd == "stepIn")
      {
         response = { {"status", "ok"} };
         SendResponse(response);
         emulation_->Step();
      }
      else if (cmd == "stepOut")
      {
         response = { {"status", "ok"} };
         SendResponse(response);
         emulation_->StepOut();
      }
      else if (cmd == "halt")
      {
         response = { {"status", "ok"} };
         SendResponse(response);
         emulation_->Break();
      }
      else if (cmd == "setPC")
      {
         uint16_t pc = static_cast<uint16_t>(request.value("address", 0));
         emulation_->GetEngine()->GetProc()->PrepareForFetch(pc);
         response = { {"status", "ok"} };
         SendResponse(response);
      }
      else if (cmd == "setRegisters")
      {
         Z80* z80 = emulation_->GetEngine()->GetProc();
         if (request.contains("pc"))  z80->PrepareForFetch(static_cast<uint16_t>(request["pc"]));
         if (request.contains("sp"))  z80->sp_    = static_cast<uint16_t>(request["sp"]);
         if (request.contains("af"))  z80->af_.w  = static_cast<uint16_t>(request["af"]);
         if (request.contains("bc"))  z80->bc_.w  = static_cast<uint16_t>(request["bc"]);
         if (request.contains("de"))  z80->de_.w  = static_cast<uint16_t>(request["de"]);
         if (request.contains("hl"))  z80->hl_.w  = static_cast<uint16_t>(request["hl"]);
         if (request.contains("ix"))  z80->ix_.w  = static_cast<uint16_t>(request["ix"]);
         if (request.contains("iy"))  z80->iy_.w  = static_cast<uint16_t>(request["iy"]);
         if (request.contains("af'")) z80->af_p_.w = static_cast<uint16_t>(request["af'"]);
         if (request.contains("bc'")) z80->bc_p_.w = static_cast<uint16_t>(request["bc'"]);
         if (request.contains("de'")) z80->de_p_.w = static_cast<uint16_t>(request["de'"]);
         if (request.contains("hl'")) z80->hl_p_.w = static_cast<uint16_t>(request["hl'"]);
         response = { {"status", "ok"} };
         SendResponse(response);
      }
      else if (cmd == "continue")
      {
         response = { {"status", "running"} };
         SendResponse(response);
         emulation_->Run();
      }
      else if (cmd == "readMemory")
      {
         HandleReadMemory (request);
      }
      else if (cmd == "writeMemory")
      {
         uint16_t address = request.value("address", 0);
         const auto& bytes = request["bytes"];
         Memory* mem = emulation_->GetEngine()->GetMem();
         for (size_t i = 0; i < bytes.size(); i++)
         {
            uint16_t dest = static_cast<uint16_t>(address + i);
            mem->SetDbg(dest, static_cast<unsigned char>(bytes[i]), dest >> 14);
         }
         response = { {"status", "ok"}, {"written", (int)bytes.size()} };
         SendResponse(response);
      }
      else if (cmd == "evaluate")
      {
         std::string expr = request.value("expression", "");
         Z80* z80 = emulation_->GetEngine()->GetProc();

         // Normalize to uppercase
         std::string up = expr;
         std::transform(up.begin(), up.end(), up.begin(), ::toupper);

         std::string text;

         auto hex16 = [](uint16_t v) -> std::string {
            char buf[8];
            snprintf(buf, sizeof(buf), "0x%04X", v);
            return buf;
         };
         auto hex8 = [](unsigned char v) -> std::string {
            char buf[6];
            snprintf(buf, sizeof(buf), "0x%02X", v);
            return buf;
         };

         if      (up == "AF")  text = hex16(z80->af_.w);
         else if (up == "AF'") text = hex16(z80->af_p_.w);
         else if (up == "BC")  text = hex16(z80->bc_.w);
         else if (up == "BC'") text = hex16(z80->bc_p_.w);
         else if (up == "DE")  text = hex16(z80->de_.w);
         else if (up == "DE'") text = hex16(z80->de_p_.w);
         else if (up == "HL")  text = hex16(z80->hl_.w);
         else if (up == "HL'") text = hex16(z80->hl_p_.w);
         else if (up == "IX")  text = hex16(z80->ix_.w);
         else if (up == "IY")  text = hex16(z80->iy_.w);
         else if (up == "SP")  text = hex16(z80->sp_);
         else if (up == "PC")  text = hex16(z80->new_instruction_ ? z80->pc_ : z80->GetPC());
         else if (up == "A")   text = hex8(z80->af_.b.h);
         else if (up == "F")   text = hex8(z80->af_.b.l);
         else if (up == "B")   text = hex8(z80->bc_.b.h);
         else if (up == "C")   text = hex8(z80->bc_.b.l);
         else if (up == "D")   text = hex8(z80->de_.b.h);
         else if (up == "E")   text = hex8(z80->de_.b.l);
         else if (up == "H")   text = hex8(z80->hl_.b.h);
         else if (up == "L")   text = hex8(z80->hl_.b.l);
         else {
            // Try to parse as [mode:]address
            // Syntax:
            //   0x4000 / 16384        → MEM_READ (default)
            //   read:0x4000           → MEM_READ
            //   write:0x4000          → MEM_WRITE
            //   ram:0x4000            → MEM_RAM_LOWER_BANK
            //   ram[N]:0x4000         → MEM_RAM_BANK, bank N
            //   rom:0x0000            → MEM_LOWER_ROM
            //   rom[N]:0x0000         → MEM_ROM_BANK, slot N
            //   cart[N]:0x0000        → MEM_CART_SLOT, slot N
            Memory::DbgMemAccess access = Memory::MEM_READ;
            unsigned int bank_data = 0;
            std::string addr_str = expr;
            bool parse_ok = true;

            auto colon = up.find(':');
            if (colon != std::string::npos) {
               std::string prefix = up.substr(0, colon);
               addr_str = expr.substr(colon + 1);

               auto lb = prefix.find('[');
               auto rb = prefix.find(']');
               if (lb != std::string::npos && rb != std::string::npos) {
                  try { bank_data = std::stoul(prefix.substr(lb + 1, rb - lb - 1)); }
                  catch (...) { parse_ok = false; }
                  prefix = prefix.substr(0, lb);
               }

               if      (prefix == "READ")  access = Memory::MEM_READ;
               else if (prefix == "WRITE") access = Memory::MEM_WRITE;
               else if (prefix == "RAM")   access = (lb != std::string::npos) ? Memory::MEM_RAM_BANK : Memory::MEM_RAM_LOWER_BANK;
               else if (prefix == "ROM")   access = (lb != std::string::npos) ? Memory::MEM_ROM_BANK : Memory::MEM_LOWER_ROM;
               else if (prefix == "CART")  access = Memory::MEM_CART_SLOT;
               else                        parse_ok = false;
            }

            if (parse_ok) {
               try {
                  uint32_t addr = std::stoul(addr_str, nullptr, 0);
                  unsigned char byte = 0;
                  emulation_->GetEngine()->GetMem()->GetDebugValue(&byte, addr & 0xFFFF, 1, access, bank_data);
                  text = hex8(byte) + " @ " + hex16(static_cast<uint16_t>(addr));
               } catch (...) { text = "?"; }
            } else {
               text = "?";
            }
         }

         response = { {"text", text} };
         SendResponse(response);
      }
      else if (cmd == "loadSnapshot")
      {
         bool ok = false;
         std::string errmsg;

         if (request.contains("data"))
         {
            // Inline base64 payload — decode to a temp file then load
            std::string b64 = request.value("data", "");
            auto bytes = Base64Decode(b64);

            // Write temp file
            std::string tmpPath;
#ifdef _WIN32
            char tmpBuf[MAX_PATH];
            GetTempPathA(MAX_PATH, tmpBuf);
            tmpPath = std::string(tmpBuf) + "sugarbox_dap.sna";
#else
            tmpPath = "/tmp/sugarbox_dap.sna";
#endif
            {
               std::ofstream f(tmpPath, std::ios::binary | std::ios::trunc);
               if (f) f.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
               else   errmsg = "cannot write temp file: " + tmpPath;
            }
            if (errmsg.empty())
            {
               ok = emulation_->LoadSnapshot(tmpPath.c_str());
               if (!ok) errmsg = "snapshot load failed (from inline data)";
               std::remove(tmpPath.c_str());
            }
         }
         else
         {
            // Legacy path-based load
            std::string path = request.value("path", "");
            ok = emulation_->LoadSnapshot(path.c_str());
            if (!ok) errmsg = "snapshot load failed: " + path;
         }

         response = { {"status", ok ? "ok" : "error"} };
         if (!ok) response["message"] = errmsg;
         SendResponse(response);
      }
      else if (cmd == "reset")
      {
         emulation_->HardReset();
         emulation_->Break();
         response = { {"status", "ok"} };
         SendResponse(response);
      }
      else if (cmd == "setBreakpoints")
      {
         emulation_->GetEngine()->CleanBreakpoints();
         const auto& bps = request.value("breakpoints", json::array());
         for (const auto& bp : bps)
         {
            uint16_t address = static_cast<uint16_t>(bp.value("address", 0));
            emulation_->GetEngine()->AddBreakpoint(address);
         }
         response = { {"status", "ok"} };
         SendResponse(response);
      }
      else if (cmd == "disassemble")
      {
         // [
         // { "address": 7842, "text": "LD A,(HL)" },
         // { "address": 7843, "text": "INC HL" }
         // ]      
         // Use uint16_t so the address wraps correctly at 0xFFFF→0x0000
         uint16_t pc    = static_cast<uint16_t>(request.value("address", 0));
         unsigned int count = request.value("count", 0);
         std::string type = request.value("type", "READ");

         json arr = json::array();
         for (unsigned int i = 0; i < count; i++)
         {
            char out_buffer[128];
            memset(out_buffer, 0x20, sizeof(out_buffer));
            int increment = emulation_->Disassemble(pc, out_buffer, 128 - 7);
            if (increment <= 0) increment = 1;   // guard against invalid opcodes

            // Read raw bytes for hex/ASCII display (max 4 bytes for any Z80 instruction)
            int byteCount = (increment <= 4) ? increment : 1;
            unsigned char rawBytes[4] = {0, 0, 0, 0};
            emulation_->ReadMemory(pc, rawBytes, byteCount);
            json bytesArr = json::array();
            for (int b = 0; b < byteCount; b++)
               bytesArr.push_back(rawBytes[b]);

            arr.push_back({
               { "address", pc },
               { "instruction", out_buffer },
               { "bytes", bytesArr }
            });
            pc += static_cast<uint16_t>(increment);   // wraps at 0xFFFF
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
void DebugServer::HandleReadMemory(const nlohmann::json& request)
{
    uint32_t address = request.value("address", 0);
    uint32_t size    = request.value("size", 0);

    // Cap to Z80 address space
    if (size > 65536) size = 65536;

    std::vector<unsigned char> buf(size);
    emulation_->ReadMemory(static_cast<uint16_t>(address & 0xFFFF), buf.data(), size);

    json bytes = json::array();
    for (uint32_t i = 0; i < size; i++)
        bytes.push_back(buf[i]);

    json response = { { "bytes", bytes } };
    SendResponse(response);
}

void DebugServer::SendResponse(json response)
{      
   std::string out = response.dump() + "\n";
   outgoing_queue_.push(out);
}
