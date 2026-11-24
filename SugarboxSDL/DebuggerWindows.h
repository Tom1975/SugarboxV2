#pragma once

#include <atomic>
#include <string>
#include <vector>

#include "Memoire.h"

class Emulation;

// Debugger windows, drawn with Dear ImGui over the emulated screen (main thread) :
// - Debugger : run / break / steps, disassembly with breakpoints, registers, stack
// - Memory 1 to 4 : hex dump of the memory seen by the Z80, or of a RAM / ROM / cartridge bank
// - CRTC : registers and counters
// The values are read while the emulation runs (for display only) ; they are changed with the
// emulation held (Emulation::CommandLock), and only when it is stopped.
class DebuggerWindows
{
public:
   static const int kNbMemoryWindows = 4;

   void Init(Emulation* emulation);

   // Debug menu
   void DrawMenu();
   void OpenDebugger();

   // Main thread, in the ImGui frame
   void Draw();
   // One of the windows has the keyboard : the keys are not for the CPC
   bool HasFocus() const { return has_focus_; }

   // The emulation stopped (breakpoint, step...) : any thread
   void NotifyStop(bool open_debugger) { stopped_ = true; open_on_stop_ = open_on_stop_ || open_debugger; }

protected:
   void DrawDebugger();
   void DrawToolbar();
   void DrawDisassembly();
   void DrawRegisters();
   void DrawStack();
   void DrawMemory(int index);
   void DrawCrtc();
   void HandleShortcuts();

   // Address of the instruction before the one at address
   unsigned short PreviousAddress(unsigned short address) const;
   // Address shown by the PC arrow : the instruction the emulation stopped on
   unsigned short CurrentPC() const;
   bool IsStopped() const;
   void ToggleBreakpoint(unsigned short address);

   Emulation* emulation_ = nullptr;
   bool has_focus_ = false;
   std::atomic<bool> stopped_{ false };
   std::atomic<bool> open_on_stop_{ false };

   // Debugger
   bool debugger_open_ = false;
   unsigned short top_address_ = 0;
   int selected_line_ = -1;
   std::vector<unsigned short> line_addresses_;
   char goto_address_[8] = {};
   bool follow_pc_ = true;

   // Memory windows
   struct MemoryView
   {
      bool open = false;
      Memory::DbgMemAccess access = Memory::MEM_READ;
      unsigned int bank = 0;
      char goto_address_[8] = {};
      int scroll_to = -1;
   };
   MemoryView memory_[kNbMemoryWindows];

   bool crtc_open_ = false;
};
