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

         response["AF"]  = z80->af_.w;
         response["AF'"] = z80->af_p_.w;
         response["BC"]  = z80->bc_.w;
         response["BC'"] = z80->bc_p_.w;
         response["DE"]  = z80->de_.w;
         response["DE'"] = z80->de_p_.w;
         response["HL"]  = z80->hl_.w;
         response["HL'"] = z80->hl_p_.w;
         response["IX"]  = z80->ix_.w;
         response["IY"]  = z80->iy_.w;
         response["SP"]  = z80->sp_;
         response["PC"]  = z80->new_instruction_ ? z80->pc_ : z80->GetPC();
         response["I"]   = z80->ir_.b.h;
         response["R"]   = z80->ir_.b.l;
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
      else if (cmd == "getMemBanks")
      {
         HandleGetMemBanks();
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
         uint16_t pc        = static_cast<uint16_t>(request.value("address", 0));
         unsigned int count = request.value("count", 0);

         // Optional source: memType = "read"|"write"|"ram"|"rom"|"cart", bank = int
         std::string memType = request.value("memType", "read");
         int         bank    = request.value("bank", -1);

         std::string up = memType;
         std::transform(up.begin(), up.end(), up.begin(), ::toupper);

         Memory* mem = emulation_->GetEngine()->GetMem();

         // Determine access mode (mirrors HandleReadMemory logic)
         Memory::DbgMemAccess access = Memory::MEM_READ;
         if      (up == "WRITE") access = Memory::MEM_WRITE;
         else if (up == "RAM"  && bank < 0) access = Memory::MEM_RAM_LOWER_BANK;
         else if (up == "RAM"  )            access = Memory::MEM_RAM_BANK;
         else if (up == "ROM"  && bank < 0) access = Memory::MEM_LOWER_ROM;
         else if (up == "ROM"  )            access = Memory::MEM_ROM_BANK;
         else if (up == "CART" )            access = Memory::MEM_CART_SLOT;

         const bool useDefaultReader = (access == Memory::MEM_READ);

         // For non-READ sources: read the whole address space into a local buffer.
         // ROM/RAM banks are 0x4000 bytes; READ/WRITE span 0x10000.
         const uint32_t BUF_SIZE = (access == Memory::MEM_READ || access == Memory::MEM_WRITE
                                    || access == Memory::MEM_RAM_LOWER_BANK)
                                   ? 0x10000u : 0x4000u;
         std::vector<unsigned char> bankBuf;
         if (!useDefaultReader)
         {
            bankBuf.resize(BUF_SIZE, 0);
            mem->GetDebugValue(bankBuf.data(), 0,
                               static_cast<uint16_t>(BUF_SIZE),
                               access,
                               static_cast<unsigned int>(bank < 0 ? 0 : bank));
         }

         Z80Desassember* dasm = emulation_->GetDisassembler();

         // Build the byte-reader lambda used for disassembly
         auto makeReader = [&]() -> Z80Desassember::ReadByteFn {
            if (useDefaultReader) {
               return [mem](unsigned short a) -> unsigned char { return mem->Get(a); };
            } else {
               return [&bankBuf, BUF_SIZE](unsigned short a) -> unsigned char {
                  return bankBuf[a % BUF_SIZE];
               };
            }
         };
         auto readByte = makeReader();

         json arr = json::array();
         for (unsigned int i = 0; i < count; i++)
         {
            char mnemonic[16], argument[16];
            int increment = dasm->DasmMnemonicEx(pc, readByte, mnemonic, argument);
            if (increment <= 0) increment = 1;

            // Format: "MNEMONIC ARGUMENT" in a fixed-width field
            char out_buffer[128];
            std::snprintf(out_buffer, sizeof(out_buffer), "%s %s", mnemonic, argument);

            // Raw bytes for hex/ASCII display column
            int byteCount = (increment <= 4) ? increment : 1;
            json bytesArr = json::array();
            for (int b = 0; b < byteCount; b++)
               bytesArr.push_back(readByte(static_cast<uint16_t>(pc + b)));

            arr.push_back({
               { "address",     pc          },
               { "instruction", out_buffer  },
               { "bytes",       bytesArr    }
            });
            pc += static_cast<uint16_t>(increment);
         }
         response = {
            { "type",         "response"    },
            { "command",      "disassemble" },
            { "instructions", arr           },
         };
         SendResponse(response);
      }
      else if (cmd == "getCrtcState")
      {
         HandleGetCrtcState();
      }
      else if (cmd == "getGateArrayState")
      {
         HandleGetGateArrayState();
      }
      else if (cmd == "getPsgState")
      {
         HandleGetPsgState();
      }
      else if (cmd == "getPpiState")
      {
         HandleGetPpiState();
      }
      else if (cmd == "getFdcState")
      {
         HandleGetFdcState();
      }
      else if (cmd == "getTapeState")
      {
         HandleGetTapeState();
      }
      else if (cmd == "getAsicState")
      {
         HandleGetAsicState();
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

    // Optional source selection: type = "read"|"write"|"ram"|"rom"|"cart"
    // bank = bank/slot index (used by ram/rom/cart)
    std::string type  = request.value("memType", "read");
    unsigned int bank = request.value("bank", 0);

    // Normalize to uppercase for comparison
    std::string up = type;
    std::transform(up.begin(), up.end(), up.begin(), ::toupper);

    Memory* mem = emulation_->GetEngine()->GetMem();
    Memory::DbgMemAccess access = Memory::MEM_READ;
    if      (up == "WRITE") access = Memory::MEM_WRITE;
    else if (up == "RAM"  && bank == (unsigned int)-1) access = Memory::MEM_RAM_LOWER_BANK;
    else if (up == "RAM"  ) access = Memory::MEM_RAM_BANK;
    else if (up == "ROM"  && bank == (unsigned int)-1) access = Memory::MEM_LOWER_ROM;
    else if (up == "ROM"  ) access = Memory::MEM_ROM_BANK;
    else if (up == "CART" ) access = Memory::MEM_CART_SLOT;

    std::vector<unsigned char> buf(size, 0);
    mem->GetDebugValue(buf.data(), static_cast<uint16_t>(address & 0xFFFF), size, access, bank);

    json bytes = json::array();
    for (uint32_t i = 0; i < size; i++)
        bytes.push_back(buf[i]);

    json response = { { "bytes", bytes } };
    SendResponse(response);
}

void DebugServer::HandleGetMemBanks()
{
    Memory* mem = emulation_->GetEngine()->GetMem();
    json sources = json::array();

    // ── Always available ──────────────────────────────────────────────────────
    sources.push_back({ {"type","read"}, {"bank",-1}, {"label","Memory (Read)"},  {"maxAddr",0xFFFF} });
    sources.push_back({ {"type","write"},{"bank",-1}, {"label","Memory (Write)"}, {"maxAddr",0xFFFF} });
    sources.push_back({ {"type","ram"}, {"bank",-1},  {"label","RAM lower bank"}, {"maxAddr",0xFFFF} });

    // ── Extended RAM banks (up to 8) ──────────────────────────────────────────
    bool* ramAvail = mem->GetAvailableRam();
    int nbRam = 0;
    for (int i = 0; i < 8; i++)
    {
        if (ramAvail && ramAvail[i])
        {
            char label[32];
            snprintf(label, sizeof(label), "RAM bank %d", i);
            sources.push_back({ {"type","ram"}, {"bank",i}, {"label",label}, {"maxAddr",0x3FFF} });
            nbRam++;
        }
    }

    // ── Lower ROM ─────────────────────────────────────────────────────────────
    bool hasLowerRom = mem->IsLowerRomLoaded();
    std::cout << "[getMemBanks] lower_rom_available=" << hasLowerRom << std::endl;
    if (hasLowerRom)
        sources.push_back({ {"type","rom"}, {"bank",-1}, {"label","Lower ROM (OS)"}, {"maxAddr",0x3FFF} });

    // ── Upper ROM banks (0-255) ───────────────────────────────────────────────
    bool* romAvail = mem->GetAvailableROM();
    int nbRom = 0;
    for (int i = 0; i < 256; i++)
    {
        if (romAvail && romAvail[i])
        {
            char label[32];
            snprintf(label, sizeof(label), "ROM bank %d", i);
            sources.push_back({ {"type","rom"}, {"bank",i}, {"label",label}, {"maxAddr",0x3FFF} });
            std::cout << "[getMemBanks] ROM bank " << i << " available" << std::endl;
            nbRom++;
        }
    }

    // ── Cartridge slots (0-31) ────────────────────────────────────────────────
    bool* cartAvail = mem->GetAvailableCartridgeSlot();
    int nbCart = 0;
    for (int i = 0; i < 32; i++)
    {
        if (cartAvail && cartAvail[i])
        {
            char label[32];
            snprintf(label, sizeof(label), "Cart slot %d", i);
            sources.push_back({ {"type","cart"}, {"bank",i}, {"label",label}, {"maxAddr",0x3FFF} });
            nbCart++;
        }
    }

    std::cout << "[getMemBanks] total: " << sources.size()
              << " (lowerROM=" << hasLowerRom
              << " upperROMs=" << nbRom
              << " extRAMs=" << nbRam
              << " cart=" << nbCart << ")" << std::endl;

    json response = { {"sources", sources} };
    SendResponse(response);
}

void DebugServer::SendResponse(json response)
{
   std::string out = response.dump() + "\n";
   outgoing_queue_.push(out);
}

// ─── Hardware state handlers ──────────────────────────────────────────────────

void DebugServer::HandleGetCrtcState()
{
    CRTC* crtc = emulation_->GetEngine()->GetCRTC();
    bool  isPlus = emulation_->GetEngine()->IsPLUS();

    json regs  = json::array();
    json masks = json::array();
    for (int i = 0; i < 18; i++) {
        regs.push_back(crtc->registers_list_[i]);
        masks.push_back(crtc->registers_mask_[i]);
    }

    json resp;
    resp["registers"]   = regs;
    resp["masks"]       = masks;
    resp["crtcType"]    = static_cast<int>(crtc->type_crtc_);
    resp["isPlus"]      = isPlus;
    resp["addrReg"]     = crtc->adddress_register_;
    resp["statusReg"]   = crtc->status_register_;
    resp["hcc"]         = crtc->hcc_;
    resp["vlc"]         = crtc->vlc_;
    resp["vcc"]         = crtc->vcc_;
    resp["vertAdj"]     = crtc->vertical_adjust_counter_;
    resp["ma"]          = crtc->ma_;
    resp["hPulse"]      = crtc->horinzontal_pulse_;
    resp["vertPulse"]   = crtc->scanline_vbl_;
    resp["r52"]         = emulation_->GetEngine()->GetVGA()->interrupt_counter_;
    resp["beamX"]       = emulation_->GetEngine()->GetMonitor()->GetX();
    resp["beamY"]       = emulation_->GetEngine()->GetMonitor()->GetY();
    SendResponse(resp);
}

void DebugServer::HandleGetGateArrayState()
{
    GateArray* ga = emulation_->GetEngine()->GetVGA();

    json inks = json::array();
    for (int i = 0; i < 16; i++)
        inks.push_back(ga->ink_list_[i]);

    json resp;
    resp["mode"]             = ga->screen_mode_;
    resp["pen"]              = ga->pen_r_;
    resp["inks"]             = inks;
    resp["border"]           = ga->video_border_[0];
    resp["interruptCounter"] = ga->interrupt_counter_;
    resp["interruptRaised"]  = ga->interrupt_raised_;
    resp["asicLocked"]       = (ga->unlocked_ == false);
    SendResponse(resp);
}

void DebugServer::HandleGetPsgState()
{
    Ay8912* psg = emulation_->GetEngine()->GetPSG();
    const unsigned char* rawRegs = psg->GetRegisters();

    json regs = json::array();
    for (int i = 0; i < 16; i++)
        regs.push_back(rawRegs[i]);

    json resp;
    resp["registers"] = regs;
    resp["chanAFreq"] = psg->GetChanAFreq();
    resp["chanBFreq"] = psg->GetChanBFreq();
    resp["chanCFreq"] = psg->GetChanCFreq();
    resp["noiseFreq"] = psg->GetNoiseFreq();
    resp["mixer"]     = psg->GetMixer();
    resp["chanAVol"]  = psg->GetChanAVol();
    resp["chanBVol"]  = psg->GetChanBVol();
    resp["chanCVol"]  = psg->GetChanCVol();
    resp["envFreq"]   = psg->GetEnvFreq();
    resp["envShape"]  = psg->GetEnvShape();
    resp["portA"]     = psg->GetPortA();
    resp["portB"]     = psg->GetPortB();
    SendResponse(resp);
}

void DebugServer::HandleGetPpiState()
{
    PPI8255* ppi = emulation_->GetEngine()->GetPPI();

    json resp;
    resp["portA"]        = ppi->port_a_;
    resp["portB"]        = ppi->port_b_;
    resp["portC"]        = ppi->port_c_;
    resp["controlWord"]  = ppi->control_word_.byte;
    SendResponse(resp);
}

void DebugServer::HandleGetFdcState()
{
    FDC* fdc = emulation_->GetEngine()->GetFDC();

    json resp;
    resp["mainStatus"]  = fdc->GetMainStatus();
    resp["status0"]     = fdc->GetDebugStatus0();
    resp["status1"]     = fdc->GetDebugStatus1();
    resp["status2"]     = fdc->GetDebugStatus2();
    resp["status3"]     = fdc->GetDebugStatus3();
    resp["currentDrive"] = fdc->GetCurrentDrive();
    resp["motorOn"]     = fdc->IsMotorOn();

    json drives = json::array();
    for (int d = 0; d < 2; d++) {
        json drv;
        drv["present"]       = fdc->IsDiskPresent(d);
        drv["writeProtected"] = fdc->IsDiskWriteProtected(d);
        drv["track"]         = fdc->GetCurrentTrack(d);
        drv["sector"]        = fdc->GetCurrentSector(d);
        drv["side"]          = fdc->GetCurrentSide(d);
        const char* path = fdc->GetDiskPath(d);
        drv["path"]          = path ? path : "";
        drives.push_back(drv);
    }
    resp["drives"] = drives;
    SendResponse(resp);
}

void DebugServer::HandleGetTapeState()
{
    CTape* tape = emulation_->GetEngine()->GetTape();

    json blocks = json::array();
    int nbBlocks = tape->GetNbBlocks();
    for (int i = 0; i < nbBlocks; i++) {
        json blk;
        blk["index"]    = i;
        blk["position"] = tape->GetBlockPosition(i);
        const char* txt = tape->GetTextBlock(i);
        blk["text"]     = txt ? txt : "";
        blocks.push_back(blk);
    }

    const char* path = tape->GetTapePath();
    json resp;
    resp["path"]      = path ? path : "";
    resp["inserted"]  = tape->IsTapeInserted();
    resp["motor"]     = tape->GetMotor();
    resp["counter"]   = tape->GetCounter();
    resp["length"]    = tape->LengthOfTape();
    resp["blocks"]    = blocks;
    SendResponse(resp);
}

void DebugServer::HandleGetAsicState()
{
    GateArray* ga = emulation_->GetEngine()->GetVGA();
    Memory*    mem = emulation_->GetEngine()->GetMem();

    json resp;
    resp["isPlus"] = ga->plus_;

    // Sprite palette (16 colors, 0xFFRRGGBB — same encoding as ink_list_)
    json spritePalette = json::array();
    for (int i = 0; i < 16; i++)
        spritePalette.push_back(ga->sprite_ink_list_[i]);
    resp["spritePalette"] = spritePalette;

    // 16 sprites: position, zoom, visibility, pixel data
    json sprites = json::array();
    for (int i = 0; i < 16; i++) {
        Memory::TSpriteInfo* info = mem->GetSpriteInfo(i);
        unsigned char*       data = mem->GetSprite(i);

        json pixels = json::array();
        for (int p = 0; p < 256; p++)
            pixels.push_back(data[p] & 0xF);

        json spr;
        spr["x"]         = info->x;
        spr["y"]         = info->y;
        spr["zoomx"]     = info->zoomx;
        spr["zoomy"]     = info->zoomy;
        spr["displayed"] = info->displayed;
        spr["pixels"]    = pixels;
        sprites.push_back(spr);
    }
    resp["sprites"] = sprites;

    // ASIC control registers
    resp["pri"]  = mem->GetPRI();
    resp["splt"] = mem->GetSPLT();
    resp["ssa"]  = ga->GetSSA();
    resp["sscr"] = mem->GetSSCR();
    resp["ivr"]  = mem->GetIVR();
    resp["dcsr"] = mem->GetDCSR();

    // DMA channels (0-2)
    json dmaChannels = json::array();
    for (int c = 0; c < 3; c++) {
        DMA* dma = emulation_->GetEngine()->GetDMA(c);
        json ch;
        ch["sar"]          = mem->GetDMAAdress(c);
        ch["ppr"]          = mem->GetDMAPrescaler(c);
        ch["currentInstr"] = dma->curent_instr_;
        ch["paused"]       = (dma->dma_cycle_ == DMA::PAUSE);
        ch["interrupt"]    = dma->interrupt_on_;
        dmaChannels.push_back(ch);
    }
    resp["dma"] = dmaChannels;

    SendResponse(resp);
}
