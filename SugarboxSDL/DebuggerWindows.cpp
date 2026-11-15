#include "DebuggerWindows.h"

#include <cstdio>
#include <cstdlib>

#include <SDL3/SDL.h>

#include "imgui.h"

#include "Emulation.h"

namespace
{
   const ImVec4 kAddressColor(0.55f, 0.75f, 1.0f, 1.0f);
   const ImVec4 kBytesColor(0.6f, 0.6f, 0.6f, 1.0f);
   const ImVec4 kMnemonicColor(1.0f, 1.0f, 1.0f, 1.0f);
   const ImVec4 kArgumentColor(0.95f, 0.75f, 0.45f, 1.0f);
   const ImU32 kBreakpointColor = IM_COL32(220, 40, 40, 255);
   const ImU32 kPcColor = IM_COL32(255, 220, 0, 255);
   const ImU32 kPcLineColor = IM_COL32(255, 220, 0, 40);

   // A debugger window is always a system window of its own, even over the main one
   void SeparateWindow()
   {
      ImGuiWindowClass window_class;
      window_class.ViewportFlagsOverrideSet = ImGuiViewportFlags_NoAutoMerge;
      ImGui::SetNextWindowClass(&window_class);
   }

   // Hexadecimal address typed : -1 if none
   int ParseAddress(const char* text)
   {
      char* end = nullptr;
      const unsigned long value = std::strtoul(text, &end, 16);
      if (end == text || value > 0xFFFF) return -1;
      return static_cast<int>(value);
   }
}

void DebuggerWindows::Init(Emulation* emulation)
{
   emulation_ = emulation;
}

void DebuggerWindows::OpenDebugger()
{
   debugger_open_ = true;
   follow_pc_ = true;
   ImGui::SetWindowFocus("Debugger");
}

void DebuggerWindows::DrawMenu()
{
   if (ImGui::MenuItem("Debugger", nullptr, debugger_open_))
   {
      if (debugger_open_) debugger_open_ = false;
      else OpenDebugger();
   }
   if (ImGui::BeginMenu("Memory"))
   {
      for (int i = 0; i < kNbMemoryWindows; i++)
      {
         char label[16];
         std::snprintf(label, sizeof(label), "Memory %d", i + 1);
         if (ImGui::MenuItem(label, nullptr, memory_[i].open)) memory_[i].open = !memory_[i].open;
      }
      ImGui::EndMenu();
   }
   if (ImGui::MenuItem("CRTC", nullptr, crtc_open_)) crtc_open_ = !crtc_open_;
   ImGui::Separator();
   const bool stopped = IsStopped();
   if (ImGui::MenuItem("Run", "F5", false, stopped)) emulation_->Run();
   if (ImGui::MenuItem("Break", "F5", false, !stopped)) emulation_->Break();
   if (ImGui::MenuItem("Step into", "F11", false, stopped)) emulation_->Step();
   if (ImGui::MenuItem("Step over", "F10", false, stopped)) emulation_->StepOver();
   if (ImGui::MenuItem("Step out", "Shift+F11", false, stopped)) emulation_->StepOut();
}

bool DebuggerWindows::IsStopped() const
{
   return !emulation_->IsStepping();
}

unsigned short DebuggerWindows::CurrentPC() const
{
   // Stopped : the instruction it stopped on (after a breakpoint, the Z80 PC is one byte further)
   return IsStopped() ? emulation_->GetDebugPC() : emulation_->GetEngine()->GetProc()->pc_;
}

void DebuggerWindows::ToggleBreakpoint(unsigned short address)
{
   const Emulation::CommandLock lock(emulation_);
   emulation_->GetEngine()->GetBreakpointHandler()->ToggleBreakpoint(address);
}

unsigned short DebuggerWindows::PreviousAddress(unsigned short address) const
{
   // The longest instruction that ends just before this address, else the shortest one that
   // starts before it (as the Qt disassembly does)
   Z80* z80 = emulation_->GetEngine()->GetProc();
   for (int size = 4; size >= 1; size--)
   {
      const unsigned short candidate = static_cast<unsigned short>(address - size);
      if (z80->GetOpcodeSize(candidate) == size) return candidate;
   }
   return static_cast<unsigned short>(address - 1);
}

void DebuggerWindows::Draw()
{
   has_focus_ = false;
   if (emulation_ == nullptr) return;

   if (stopped_.exchange(false))
   {
      follow_pc_ = true;
      if (open_on_stop_.exchange(false)) OpenDebugger();
   }

   if (debugger_open_) DrawDebugger();
   for (int i = 0; i < kNbMemoryWindows; i++)
   {
      if (memory_[i].open) DrawMemory(i);
   }
   if (crtc_open_) DrawCrtc();
}

void DebuggerWindows::HandleShortcuts()
{
   const bool stopped = IsStopped();
   if (ImGui::IsKeyPressed(ImGuiKey_F5, false))
   {
      if (stopped) emulation_->Run();
      else emulation_->Break();
   }
   if (!stopped) return;
   if (ImGui::IsKeyPressed(ImGuiKey_F10, false)) emulation_->StepOver();
   if (ImGui::IsKeyPressed(ImGuiKey_F11, false))
   {
      if (ImGui::GetIO().KeyShift) emulation_->StepOut();
      else emulation_->Step();
   }
}

///////////////////////////////////////////////////
// Debugger

void DebuggerWindows::DrawDebugger()
{
   const float font = ImGui::GetFontSize();
   const ImVec2 origin = ImGui::GetMainViewport()->WorkPos;
   ImGui::SetNextWindowPos(ImVec2(origin.x + font, origin.y + font * 2.0f), ImGuiCond_FirstUseEver);
   ImGui::SetNextWindowSize(ImVec2(font * 52.0f, font * 34.0f), ImGuiCond_FirstUseEver);
   SeparateWindow();
   if (!ImGui::Begin("Debugger", &debugger_open_, ImGuiWindowFlags_NoCollapse))
   {
      ImGui::End();
      return;
   }
   if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
   {
      has_focus_ = true;
      HandleShortcuts();
   }

   DrawToolbar();
   ImGui::Separator();

   const float right_width = font * 15.0f;
   if (ImGui::BeginChild("##dasm", ImVec2(-right_width, 0.0f), ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
   {
      DrawDisassembly();
   }
   ImGui::EndChild();
   ImGui::SameLine();
   if (ImGui::BeginChild("##state", ImVec2(0.0f, 0.0f)))
   {
      DrawRegisters();
      DrawStack();
   }
   ImGui::EndChild();

   ImGui::End();
}

void DebuggerWindows::DrawToolbar()
{
   const bool stopped = IsStopped();
   if (stopped)
   {
      if (ImGui::Button("Run (F5)")) emulation_->Run();
   }
   else
   {
      if (ImGui::Button("Break (F5)")) emulation_->Break();
   }
   ImGui::SameLine();
   ImGui::BeginDisabled(!stopped);
   if (ImGui::Button("Step into (F11)")) emulation_->Step();
   ImGui::SameLine();
   if (ImGui::Button("Step over (F10)")) emulation_->StepOver();
   ImGui::SameLine();
   if (ImGui::Button("Step out (Shift+F11)")) emulation_->StepOut();
   ImGui::EndDisabled();

   ImGui::SameLine(0.0f, ImGui::GetFontSize() * 2.0f);
   if (stopped) ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Break at %04X", CurrentPC());
   else ImGui::TextDisabled("Running");

   ImGui::AlignTextToFramePadding();
   ImGui::TextUnformatted("Go to");
   ImGui::SameLine();
   ImGui::SetNextItemWidth(ImGui::GetFontSize() * 4.0f);
   const bool enter = ImGui::InputText("##goto", goto_address_, sizeof(goto_address_),
      ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_EnterReturnsTrue);
   ImGui::SameLine();
   if ((ImGui::Button("Go") || enter) && ParseAddress(goto_address_) >= 0)
   {
      top_address_ = static_cast<unsigned short>(ParseAddress(goto_address_));
      follow_pc_ = false;
   }
   ImGui::SameLine();
   if (ImGui::Button("PC")) follow_pc_ = true;
   ImGui::SameLine();
   ImGui::TextDisabled("Margin click / F9 : breakpoint");
}

void DebuggerWindows::DrawDisassembly()
{
   EmulatorEngine* engine = emulation_->GetEngine();
   Z80Desassember* disassembler = emulation_->GetDisassembler();
   BreakpointHandler* breakpoints = engine->GetBreakpointHandler();
   const float line_height = ImGui::GetTextLineHeightWithSpacing();
   const int nb_lines = SDL_max(1, static_cast<int>(ImGui::GetContentRegionAvail().y / line_height));
   const unsigned short pc = CurrentPC();

   // The PC in view, with a few lines above it
   if (follow_pc_)
   {
      follow_pc_ = false;
      bool visible = false;
      for (size_t i = 0; i + 2 < line_addresses_.size() && !visible; i++)
      {
         visible = line_addresses_[i] == pc && i >= 2;
      }
      if (!visible)
      {
         top_address_ = pc;
         for (int i = 0; i < 3; i++) top_address_ = PreviousAddress(top_address_);
      }
   }

   // Mouse wheel, keys
   const bool hovered = ImGui::IsWindowHovered();
   const bool focused = ImGui::IsWindowFocused();
   int scroll = 0;
   if (hovered) scroll = -static_cast<int>(ImGui::GetIO().MouseWheel * 3.0f);
   if (focused)
   {
      if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) scroll += 1;
      if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) scroll -= 1;
      if (ImGui::IsKeyPressed(ImGuiKey_PageDown)) scroll += nb_lines - 1;
      if (ImGui::IsKeyPressed(ImGuiKey_PageUp)) scroll -= nb_lines - 1;
      if (ImGui::IsKeyPressed(ImGuiKey_F9, false) && selected_line_ >= 0 && selected_line_ < static_cast<int>(line_addresses_.size()))
      {
         ToggleBreakpoint(line_addresses_[selected_line_]);
      }
   }
   for (; scroll < 0; scroll++) top_address_ = PreviousAddress(top_address_);
   for (; scroll > 0; scroll--)
   {
      char mnemonic[16], argument[16];
      top_address_ = static_cast<unsigned short>(top_address_ + SDL_max(1, disassembler->DasmMnemonic(top_address_, mnemonic, argument)));
   }

   ImDrawList* draw = ImGui::GetWindowDrawList();
   const ImVec2 origin = ImGui::GetCursorScreenPos();
   const float width = ImGui::GetContentRegionAvail().x;
   const float margin = ImGui::GetFontSize() * 1.4f;
   const float char_width = ImGui::CalcTextSize("0").x;

   line_addresses_.clear();
   unsigned short address = top_address_;
   for (int line = 0; line < nb_lines; line++)
   {
      line_addresses_.push_back(address);
      char mnemonic[16] = {}, argument[16] = {};
      const int size = SDL_max(1, disassembler->DasmMnemonic(address, mnemonic, argument));
      const float y = origin.y + line * line_height;
      const ImVec2 line_min(origin.x, y);
      const ImVec2 line_max(origin.x + width, y + line_height);

      // Click : in the margin, a breakpoint ; elsewhere, the line selected
      ImGui::SetCursorScreenPos(line_min);
      ImGui::PushID(line);
      if (ImGui::InvisibleButton("##line", ImVec2(width, line_height)))
      {
         if (ImGui::GetIO().MousePos.x < origin.x + margin) ToggleBreakpoint(address);
         else selected_line_ = line;
      }
      ImGui::PopID();

      if (address == pc) draw->AddRectFilled(line_min, line_max, kPcLineColor);
      if (line == selected_line_) draw->AddRectFilled(line_min, line_max, ImGui::GetColorU32(ImGuiCol_Header));

      // Margin : breakpoint, PC arrow
      const float center_y = y + line_height * 0.5f;
      if (breakpoints->IsThereBreakOnAdress(address))
      {
         draw->AddCircleFilled(ImVec2(origin.x + margin * 0.35f, center_y), line_height * 0.3f, kBreakpointColor);
      }
      if (address == pc)
      {
         const float x = origin.x + margin * 0.55f;
         const float half = line_height * 0.3f;
         draw->AddTriangleFilled(ImVec2(x, center_y - half), ImVec2(x, center_y + half), ImVec2(x + half * 1.3f, center_y), kPcColor);
      }

      // Address, bytes, instruction
      char text[32];
      float x = origin.x + margin;
      std::snprintf(text, sizeof(text), "%04X", address);
      draw->AddText(ImVec2(x, y), ImGui::GetColorU32(kAddressColor), text);
      x += char_width * 6.0f;
      text[0] = 0;
      for (int i = 0; i < size && i < 4; i++)
      {
         char byte[4];
         std::snprintf(byte, sizeof(byte), "%02X ", engine->GetMem()->Get(static_cast<unsigned short>(address + i)));
         SDL_strlcat(text, byte, sizeof(text));
      }
      draw->AddText(ImVec2(x, y), ImGui::GetColorU32(kBytesColor), text);
      x += char_width * 13.0f;
      draw->AddText(ImVec2(x, y), ImGui::GetColorU32(kMnemonicColor), mnemonic);
      x += char_width * 6.0f;
      draw->AddText(ImVec2(x, y), ImGui::GetColorU32(kArgumentColor), argument);

      address = static_cast<unsigned short>(address + size);
   }
}

void DebuggerWindows::DrawRegisters()
{
   Z80* z80 = emulation_->GetEngine()->GetProc();
   const bool stopped = IsStopped();
   struct Register16
   {
      const char* name;
      unsigned short* value;
   };
   const Register16 registers[] = {
      { "AF", &z80->af_.w }, { "BC", &z80->bc_.w }, { "DE", &z80->de_.w }, { "HL", &z80->hl_.w },
      { "AF'", &z80->af_p_.w }, { "BC'", &z80->bc_p_.w }, { "DE'", &z80->de_p_.w }, { "HL'", &z80->hl_p_.w },
      { "IX", &z80->ix_.w }, { "IY", &z80->iy_.w }, { "SP", &z80->sp_ }, { "IR", &z80->ir_.w },
   };

   ImGui::SeparatorText("Registers");
   if (ImGui::BeginTable("##registers", 4, ImGuiTableFlags_SizingFixedFit))
   {
      int column = 0;
      for (const auto& reg : registers)
      {
         if (column == 0) ImGui::TableNextRow();
         ImGui::TableNextColumn();
         ImGui::AlignTextToFramePadding();
         ImGui::TextUnformatted(reg.name);
         ImGui::TableNextColumn();
         ImGui::PushID(reg.name);
         // Changed only when stopped, with the emulation held
         unsigned short value = *reg.value;
         ImGui::SetNextItemWidth(ImGui::GetFontSize() * 3.2f);
         ImGui::BeginDisabled(!stopped);
         if (ImGui::InputScalar("##v", ImGuiDataType_U16, &value, nullptr, nullptr, "%04X",
            ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue))
         {
            const Emulation::CommandLock lock(emulation_);
            *reg.value = value;
         }
         ImGui::EndDisabled();
         ImGui::PopID();
         column = (column + 1) % 2;
      }
      ImGui::EndTable();
   }

   ImGui::Text("PC  %04X", CurrentPC());
   const unsigned char f = z80->af_.b.l;
   char flags[9];
   const char* names = "SZ5H3VNC";
   for (int i = 0; i < 8; i++) flags[i] = (f & (0x80 >> i)) ? names[i] : '-';
   flags[8] = 0;
   ImGui::Text("F   %s", flags);
   ImGui::Text("IM %d  IFF1 %d  IFF2 %d", z80->interrupt_mode_, z80->iff1_ ? 1 : 0, z80->iff2_ ? 1 : 0);
}

void DebuggerWindows::DrawStack()
{
   EmulatorEngine* engine = emulation_->GetEngine();
   const unsigned short sp = engine->GetProc()->sp_;
   ImGui::SeparatorText("Stack");
   for (int i = -2; i < 10; i++)
   {
      const unsigned short address = static_cast<unsigned short>(sp + i * 2);
      const unsigned short value = static_cast<unsigned short>(engine->GetMem()->Get(address) | (engine->GetMem()->Get(static_cast<unsigned short>(address + 1)) << 8));
      if (i == 0) ImGui::TextColored(kArgumentColor, "%04X  %04X  <- SP", address, value);
      else ImGui::TextDisabled("%04X  %04X", address, value);
      // Double click : this address in the disassembly
      if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
      {
         top_address_ = value;
         follow_pc_ = false;
      }
   }
   ImGui::TextDisabled("Double click : disassemble there");
}

///////////////////////////////////////////////////
// Memory

void DebuggerWindows::DrawMemory(int index)
{
   MemoryView& view = memory_[index];
   Memory* memory = emulation_->GetEngine()->GetMem();
   char title[32];
   std::snprintf(title, sizeof(title), "Memory %d", index + 1);
   const float font = ImGui::GetFontSize();
   // Side by side under the debugger, each one a little further
   const ImVec2 origin = ImGui::GetMainViewport()->WorkPos;
   ImGui::SetNextWindowPos(ImVec2(origin.x + font * (3.0f + index * 2.0f), origin.y + font * (38.0f + index * 2.0f)), ImGuiCond_FirstUseEver);
   ImGui::SetNextWindowSize(ImVec2(font * 46.0f, font * 24.0f), ImGuiCond_FirstUseEver);
   SeparateWindow();
   if (!ImGui::Begin(title, &view.open, ImGuiWindowFlags_NoCollapse))
   {
      ImGui::End();
      return;
   }
   if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
   {
      has_focus_ = true;
      HandleShortcuts();
   }

   // Source : what the Z80 reads or writes, or a bank
   struct Source
   {
      Memory::DbgMemAccess access;
      unsigned int bank;
      std::string label;
   };
   std::vector<Source> sources = {
      { Memory::MEM_READ, 0, "Z80 read" },
      { Memory::MEM_WRITE, 0, "Z80 write" },
      { Memory::MEM_RAM_LOWER_BANK, 0, "RAM : lower 64 KB" },
   };
   bool* ram = memory->GetAvailableRam();
   if (ram[0]) sources.push_back({ Memory::MEM_RAM_BANK, 0, "RAM : upper 64 KB" });
   for (unsigned int i = 1; i < 8; i++)
   {
      if (ram[i]) sources.push_back({ Memory::MEM_RAM_BANK, i, "RAM : expansion " + std::to_string(i) });
   }
   if (memory->IsLowerRomLoaded()) sources.push_back({ Memory::MEM_LOWER_ROM, 0, "Lower ROM" });
   bool* roms = memory->GetAvailableROM();
   for (unsigned int i = 0; i < 32; i++)
   {
      if (roms[i]) sources.push_back({ Memory::MEM_ROM_BANK, i, "ROM " + std::to_string(i) });
   }
   bool* carts = memory->GetAvailableCartridgeSlot();
   for (unsigned int i = 0; i < 32; i++)
   {
      if (carts[i]) sources.push_back({ Memory::MEM_CART_SLOT, i, "Cartridge bank " + std::to_string(i) });
   }

   const char* current = "?";
   for (const auto& source : sources)
   {
      if (source.access == view.access && source.bank == view.bank) current = source.label.c_str();
   }
   ImGui::SetNextItemWidth(font * 12.0f);
   if (ImGui::BeginCombo("##source", current))
   {
      for (const auto& source : sources)
      {
         const bool selected = source.access == view.access && source.bank == view.bank;
         if (ImGui::Selectable(source.label.c_str(), selected))
         {
            view.access = source.access;
            view.bank = source.bank;
         }
      }
      ImGui::EndCombo();
   }
   ImGui::SameLine();
   ImGui::AlignTextToFramePadding();
   ImGui::TextUnformatted("Go to");
   ImGui::SameLine();
   ImGui::SetNextItemWidth(font * 4.0f);
   const bool enter = ImGui::InputText("##goto", view.goto_address_, sizeof(view.goto_address_),
      ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_EnterReturnsTrue);
   ImGui::SameLine();
   if ((ImGui::Button("Go") || enter) && ParseAddress(view.goto_address_) >= 0)
   {
      view.scroll_to = ParseAddress(view.goto_address_);
   }

   // 16 bytes per line, only the lines in view are read
   const unsigned int max_address = memory->GetDebugMaxAdress(view.access);
   const int nb_lines = static_cast<int>((max_address + 16) / 16);
   if (ImGui::BeginChild("##dump", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders))
   {
      const float line_height = ImGui::GetTextLineHeightWithSpacing();
      if (view.scroll_to >= 0)
      {
         ImGui::SetScrollY((view.scroll_to / 16) * line_height);
         view.scroll_to = -1;
      }
      ImGuiListClipper clipper;
      clipper.Begin(nb_lines, line_height);
      while (clipper.Step())
      {
         for (int line = clipper.DisplayStart; line < clipper.DisplayEnd; line++)
         {
            const unsigned short address = static_cast<unsigned short>(line * 16);
            unsigned char bytes[16] = {};
            memory->GetDebugValue(bytes, address, 16, view.access, view.bank);

            char hex[16 * 3 + 2] = {};
            char ascii[17] = {};
            for (int i = 0; i < 16; i++)
            {
               std::snprintf(hex + i * 3, 4, "%02X ", bytes[i]);
               ascii[i] = (bytes[i] >= 0x20 && bytes[i] < 0x7F) ? static_cast<char>(bytes[i]) : '.';
            }
            ImGui::TextColored(kAddressColor, "%04X", address);
            ImGui::SameLine();
            ImGui::TextUnformatted(hex);
            ImGui::SameLine();
            ImGui::TextDisabled("%s", ascii);
         }
      }
   }
   ImGui::EndChild();
   ImGui::End();
}

///////////////////////////////////////////////////
// CRTC

void DebuggerWindows::DrawCrtc()
{
   EmulatorEngine* engine = emulation_->GetEngine();
   CRTC* crtc = engine->GetCRTC();
   const bool stopped = IsStopped();
   const float font = ImGui::GetFontSize();
   // Right of the debugger
   const ImVec2 origin = ImGui::GetMainViewport()->WorkPos;
   ImGui::SetNextWindowPos(ImVec2(origin.x + font * 55.0f, origin.y + font * 2.0f), ImGuiCond_FirstUseEver);
   ImGui::SetNextWindowSize(ImVec2(font * 36.0f, font * 24.0f), ImGuiCond_FirstUseEver);
   SeparateWindow();
   if (!ImGui::Begin("CRTC", &crtc_open_, ImGuiWindowFlags_NoCollapse))
   {
      ImGui::End();
      return;
   }
   if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
   {
      has_focus_ = true;
      HandleShortcuts();
   }

   ImGui::Text("Type %d", static_cast<int>(crtc->type_crtc_));
   if (!stopped)
   {
      ImGui::SameLine();
      ImGui::TextDisabled("(changed when stopped)");
   }

   // An 8 bits value, changed with the emulation held
   auto edit8 = [&](const char* label, unsigned char* value) {
      ImGui::TableNextColumn();
      ImGui::AlignTextToFramePadding();
      ImGui::TextUnformatted(label);
      ImGui::TableNextColumn();
      ImGui::PushID(label);
      unsigned char edited = *value;
      ImGui::SetNextItemWidth(font * 2.4f);
      ImGui::BeginDisabled(!stopped);
      if (ImGui::InputScalar("##v", ImGuiDataType_U8, &edited, nullptr, nullptr, "%02X",
         ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue))
      {
         const Emulation::CommandLock lock(emulation_);
         *value = edited;
      }
      ImGui::EndDisabled();
      ImGui::PopID();
   };

   ImGui::SeparatorText("Registers");
   if (ImGui::BeginTable("##regs", 8, ImGuiTableFlags_SizingFixedFit))
   {
      for (int i = 0; i < 18; i++)
      {
         if (i % 4 == 0) ImGui::TableNextRow();
         char label[8];
         std::snprintf(label, sizeof(label), "R%d", i);
         edit8(label, &crtc->registers_list_[i]);
      }
      ImGui::EndTable();
   }

   ImGui::SeparatorText("Counters");
   if (ImGui::BeginTable("##counters", 6, ImGuiTableFlags_SizingFixedFit))
   {
      ImGui::TableNextRow();
      edit8("HCC", &crtc->hcc_);
      edit8("VLC", &crtc->vlc_);
      edit8("VCC", &crtc->vcc_);
      ImGui::TableNextRow();
      edit8("R52", &engine->GetVGA()->interrupt_counter_);
      edit8("Vert. adjust", &crtc->vertical_adjust_counter_);
      edit8("Address reg.", &crtc->adddress_register_);
      ImGui::TableNextRow();
      edit8("Status reg.", &crtc->status_register_);
      edit8("H. pulse", &crtc->horinzontal_pulse_);
      edit8("V. pulse", &crtc->scanline_vbl_);
      ImGui::EndTable();
   }
   ImGui::Text("MA  %04X", crtc->ma_);
   ImGui::Text("Beam  X %d  Y %d", engine->GetMonitor()->GetX(), engine->GetMonitor()->GetY());

   ImGui::End();
}
