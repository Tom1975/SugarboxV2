#pragma once

#include "BreakpointHandler.h"
#include <string>

class Z80Desassember
{
public:
   Z80Desassember(EmulatorEngine* machine);
   virtual ~Z80Desassember(void);

   void InitOpcodeShortcuts();

   const int DasmMnemonic(unsigned short Addr, char pMnemonic[16], char pArgument[16]) const;

   unsigned short GetMaxedPreviousValidAdress(unsigned short Addr_P);
   unsigned short GetPreviousValidAdress(unsigned short Addr_P);

   int GetLine(int y);

   bool IsFlagOnAdress(unsigned short addr);
   void ToggleFlag(unsigned short addr);

   bool IsBreakOnAdress(unsigned short addr);
   void ToggleBreakpoint(unsigned short addr);

   void GoToPrevFlag();
   void GoToNextFlag();
   void RemoveAllFlags();

   ///////////////////////////////
   // Flags
#define NB_MAX_FLAGS 256

   EmulatorEngine* machine_;
   int m_NbFlags;
   unsigned short m_Flags[NB_MAX_FLAGS];
   int m_IndexFlag;

   BreakpointHandler * breakpoint_handler_;

#define MAX_MSTATE_NB 6
#define MAX_DISASSEMBLY_SIZE 32
   struct Opcode
   {
      unsigned char Size;
      std::string Disassembly;
   };

   Opcode FillStructOpcode(unsigned char Size, const char* Disassembly)
   {
      Opcode op;
      op.Size = Size;
      op.Disassembly = Disassembly;
      return op;
   };

   Opcode ListeOpcodes[256];
   //T_PtrOpcode ListeOpcodes[256];
   Opcode ListeOpcodesCB[256];
   Opcode ListeOpcodesED[256];
   Opcode ListeOpcodesDD[256];
   Opcode ListeOpcodesFD[256];
};

