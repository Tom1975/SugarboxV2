#include "DisassemblyWidget.h"

#include <QPainter>
#include <QResizeEvent>

DisassemblyWidget::DisassemblyWidget(QWidget* parent )
   : QWidget(parent),
   disasm_window_(this),
   vertical_sb_(Qt::Vertical, this),
   horizontal_sb_(Qt::Horizontal, this),
   machine_(nullptr),
   max_adress_(0xFFFF),
   current_address_(0),
   nb_lines_(0)
{
   connect(&vertical_sb_, SIGNAL(valueChanged(int)), this, SLOT(OnValueChange(int)));

   InitOpcodeShortcuts();
}

void DisassemblyWidget::SetDisassemblyInfo(Emulation* machine, unsigned int max_adress)
{
   machine_ = machine->GetEngine();
   max_adress_ = max_adress;
}

void DisassemblyWidget::paintEvent(QPaintEvent* /* event */)
{
   QPainter painter(this);
   int width = size().width() - 3;
   int height = size().height() - 5;

   painter.fillRect(0, 0, width, height, QColor(220, 220, 220));

   // Draw every lines 
   unsigned short line_addres = current_address_;
   line_address_.clear();
   char mnemonic[16];
   char address[16];
   char arg[16];     

   sprintf(address, "0x%4.4X: ", 0xFFFF);
   QFontMetrics fm(property("font").value<QFont>());
   unsigned int address_size = fm.horizontalAdvance(address);
   unsigned int char_size = fm.horizontalAdvance(' ');
   for (unsigned int i = 0; i < nb_lines_; i++)
   {
      // Display flag ?
      // Display breakpoint ?

      // Address 
      sprintf(address, "0x%4.4X: ", line_addres);
      painter.drawText(10, 10 + line_height_ * i, address);

      // Mnemonic
      int size = DasmMnemonic(line_addres, mnemonic, arg);
      painter.drawText(10 + address_size, 10 + line_height_ * i, mnemonic);
      // Arguments
      unsigned int mnemonic_size = fm.horizontalAdvance(mnemonic);
      painter.drawText(10 + address_size + mnemonic_size + char_size, 10 + line_height_ * i, arg);

      // Bytes 
      char byte_buffer[16] = { 0 };
      char char_buffer[6] = { 0 };
      for (int j = 0; j < size && j < 5; j++)
      {
         char byte[4] = { 0 };
         unsigned char b = machine_->GetMem()->Get(line_addres + j);
         sprintf(byte, "%2.2X ", b);
         strcat(byte_buffer, byte);

         if (b >= 0x20 && b <= 126)
         {
            char_buffer[j] = b;
         }
         else
         {
            char_buffer[j] = '.';
         }
      }
      painter.drawText(200, 10 + line_height_ * i, byte_buffer);
      painter.drawText(200 + char_size*15, 10 + line_height_ * i, char_buffer);

      // Character (if displayable)
      // Current selected line

      line_address_.push_back(line_addres);

      line_addres += size;
   }

}

void DisassemblyWidget::resizeEvent(QResizeEvent* e)
{
   // If the widget's width has changed, we recalculate the new height
   // of our widget.
   if (e->size().width() == e->oldSize().width()) {
      return;
   }

   // Set scrollbar   
   vertical_sb_.move(e->size().width() - vertical_sb_.sizeHint().width(), 0);
   vertical_sb_.resize(vertical_sb_.sizeHint().width(), e->size().height() - horizontal_sb_.sizeHint().height());

   horizontal_sb_.move(0, e->size().height() - horizontal_sb_.sizeHint().height());
   horizontal_sb_.resize(e->size().width() - vertical_sb_.sizeHint().width(), horizontal_sb_.sizeHint().height());

   ComputeScrollArea();

}

void DisassemblyWidget::OnValueChange(int valueScrollBar)
{
   current_address_ = static_cast<unsigned int>(valueScrollBar);
   // Update;
   repaint();
}


void DisassemblyWidget::ComputeScrollArea()
{
   // Number of displayed lines
   QFontMetrics fm (property("font").value<QFont>());

   line_height_ = fm.lineSpacing();
   nb_lines_ = (size().height() - horizontal_sb_.sizeHint().height()) / line_height_;
   line_address_.resize(nb_lines_);

   // Vertical:
   // Compute number of lines : 0 -> max_adress_
   vertical_sb_.setMaximum(max_adress_ - (nb_lines_-1));
   vertical_sb_.setMinimum(0);
   vertical_sb_.setSingleStep(1);
   vertical_sb_.setPageStep(nb_lines_);

}


const int DisassemblyWidget::DasmMnemonic(unsigned short Addr, char pMnemonic[16], char pArgument[16]) const
{
   // Disassemble the memory from Addr, to Addr+size
   unsigned short currentAddr = Addr;

   memset(pMnemonic, 0, 16);
   memset(pArgument, 0, 16);
   if (machine_->GetMem() == NULL)
   {
      return 1;
   }
   unsigned char nextInstr_L = machine_->GetMem()->Get(currentAddr);
   unsigned char size = ListeOpcodes[nextInstr_L].Size;

   // Disassembly
   const char* Opcode_L;
   if (strcmp(ListeOpcodes[nextInstr_L].Disassembly, "%CB") == 0)
   {
      // CB ?
      nextInstr_L = machine_->GetMem()->Get(currentAddr + 1);
      Opcode_L = ListeOpcodesCB[nextInstr_L].Disassembly;
      size += ListeOpcodesCB[nextInstr_L].Size;
   }
   else if (strcmp(ListeOpcodes[nextInstr_L].Disassembly, "%ED") == 0)
   {
      // ED ?
      nextInstr_L = machine_->GetMem()->Get(currentAddr + 1);
      Opcode_L = ListeOpcodesED[nextInstr_L].Disassembly;
      size += ListeOpcodesED[nextInstr_L].Size;
   }
   else if (strcmp(ListeOpcodes[nextInstr_L].Disassembly, "%DD") == 0)
   {
      // DD
      nextInstr_L = machine_->GetMem()->Get(currentAddr + 1);
      Opcode_L = ListeOpcodesDD[nextInstr_L].Disassembly;
      size += ListeOpcodesDD[nextInstr_L].Size;
   }
   else if (strcmp(ListeOpcodes[nextInstr_L].Disassembly, "%FD") == 0)
   {
      // FD
      nextInstr_L = machine_->GetMem()->Get(currentAddr + 1);
      Opcode_L = ListeOpcodesFD[nextInstr_L].Disassembly;
      size += ListeOpcodesFD[nextInstr_L].Size;
   }
   else
   {
      Opcode_L = ListeOpcodes[nextInstr_L].Disassembly;
   }

   // First word of Disassembly is opcode
   const char* pEndOfMnemonic = strchr(Opcode_L, ' ');

   if (pEndOfMnemonic != NULL)
   {
      memcpy(pMnemonic, Opcode_L, (pEndOfMnemonic - Opcode_L) * sizeof(char));

      char* pAddr = pArgument;
      strcpy(pAddr, &pEndOfMnemonic[1]);

      //  Next
      char* pReplace_L = strchr(pAddr, '%');
      while (pReplace_L != NULL)
      {
         // Replace %nn__ by adress
         if (pReplace_L[1] == 'n' && pReplace_L[2] == 'n')
         {
            char minibuf[6];
            sprintf(minibuf, "$%2.2X%2.2X", machine_->GetMem()->Get(currentAddr + size - 1), machine_->GetMem()->Get(currentAddr + size - 2));
            // 2 then 1
            memcpy(pReplace_L, minibuf, 5 * sizeof (char));
         }
         // Replace %n2 by value
         else if (pReplace_L[1] == 'n' && pReplace_L[2] == '2')
         {
            char minibuf[4];
            sprintf(minibuf, "%2.2X ", machine_->GetMem()->Get(currentAddr + size - 2));
            // 2 then 1
            memcpy(pReplace_L, minibuf, 3 * sizeof (char));
         }
         // Replace  %n" by value
         else if (pReplace_L[1] == 'n')
         {
            char minibuf[3];
            sprintf(minibuf, "%2.2X", machine_->GetMem()->Get(currentAddr + size - 1));
            // 2 then 1
            memcpy(pReplace_L, minibuf, 2 * sizeof (char));
         }
         // Replace %j__ by relative jump
         else if (pReplace_L[1] == 'j')
         {
            char minibuf[5];
            char dec_L = machine_->GetMem()->Get(currentAddr + size - 1);
            unsigned short relative_adress = Addr + dec_L + size;
            sprintf(minibuf, "%4.4X", relative_adress);
            // 2 then 1
            memcpy(pReplace_L, minibuf, 4 * sizeof (char));

         }
         // Replace %r by register
         else if (pReplace_L[1] == 'r')
         {
            // get previous value
            int indexReg = currentAddr + size - 1;
            unsigned char reg = machine_->GetMem()->Get(indexReg);
            // mask the 3 first bits
            switch (reg & 0x7)
            {
               // convert to register
            case 00: memcpy(pReplace_L, "B ", 2 * sizeof(char)); break;
            case 01: memcpy(pReplace_L, "C ", 2 * sizeof(char)); break;
            case 02: memcpy(pReplace_L, "D ", 2 * sizeof(char)); break;
            case 03: memcpy(pReplace_L, "E ", 2 * sizeof(char)); break;
            case 04: memcpy(pReplace_L, "H ", 2 * sizeof(char)); break;
            case 05: memcpy(pReplace_L, "L ", 2 * sizeof(char)); break;
            case 06: memcpy(pReplace_L, "HL", 2 * sizeof(char)); break;
            case 07: memcpy(pReplace_L, "A ", 2 * sizeof(char)); break;
            }

         }
         // Replace %r by bit
         else if (pReplace_L[1] == 'b')
         {
            // get previous value
            int indexReg = currentAddr + size - 1;
            unsigned char bit = machine_->GetMem()->Get(indexReg);
            bit = (bit >> 3) & 0x7;
            ((char*)pReplace_L)[0] = '0' + bit;
            ((char*)pReplace_L)[1] = ' ';
         }
         pReplace_L = strchr(pAddr, '%');
      }
   }
   else
   {
      strcpy(pMnemonic, Opcode_L);
   }

   return size;
}


static unsigned char GetBitOpcode(unsigned char b, const char* r)
{
   unsigned char op = 0;
   switch (*r)
   {
   case 'B': op = 0; break;
   case 'C': op = 1; break;
   case 'D': op = 2; break;
   case 'E': op = 3; break;
   case 'L': op = 5; break;
   case 'A': op = 7; break;
   case 'H':
   {
      if (r[1] == 0)
      {
         op = 4;
      }
      else
      {
         op = 6;// HL !
      }
      break;
   };
   }
   op += (b << 3);
   return op;
}

#define DEF_OP_BIT_CPLT(o,b,r)\
{char *Buffer_Tmp = new char[64];sprintf(Buffer_Tmp, "BIT %i, %s", b, #r);   \
   ListeOpcodesCB[o] = FillStructOpcode( 1, Buffer_Tmp);delete []Buffer_Tmp;\
   }

#define DEF_OP_BIT(r)\
   {\
      unsigned opcode = GetBitOpcode (0, #r);DEF_OP_BIT_CPLT( opcode|0x40, 0, r);\
      opcode = GetBitOpcode (1, #r);DEF_OP_BIT_CPLT( opcode|0x40, 1, r);\
      opcode = GetBitOpcode (2, #r);DEF_OP_BIT_CPLT( opcode|0x40, 2, r);\
      opcode = GetBitOpcode (3, #r);DEF_OP_BIT_CPLT( opcode|0x40, 3, r);\
      opcode = GetBitOpcode (4, #r);DEF_OP_BIT_CPLT( opcode|0x40, 4, r);\
      opcode = GetBitOpcode (5, #r);DEF_OP_BIT_CPLT( opcode|0x40, 5, r);\
      opcode = GetBitOpcode (6, #r);DEF_OP_BIT_CPLT( opcode|0x40, 6, r);\
      opcode = GetBitOpcode (7, #r);DEF_OP_BIT_CPLT( opcode|0x40, 7, r);\
   }

void DisassemblyWidget::InitOpcodeShortcuts()
{
   // Par defaut, tout le monde pointe sur NOP
   for (unsigned int i = 0; i < 256; i++)
   {
      ListeOpcodes[i] = FillStructOpcode(1, "UNKNOWN");
      ListeOpcodesCB[i] = FillStructOpcode(1, "UNKNOWN");
      ListeOpcodesED[i] = FillStructOpcode(1, "UNKNOWN");
      ListeOpcodesDD[i] = FillStructOpcode(1, "UNKNOWN");
      ListeOpcodesFD[i] = FillStructOpcode(1, "UNKNOWN");
   }

   // Opcodes standards
   ///////////////////////////////////////////////////////////////////////////////////////
   /////////////                         SIZE   DISASSMBLY
   ListeOpcodes[0x00] = FillStructOpcode(1, "NOP");
   ListeOpcodes[0x01] = FillStructOpcode(3, "LD BC, %nn__");
   ListeOpcodes[0x02] = FillStructOpcode(1, "LD (BC), A");
   ListeOpcodes[0x03] = FillStructOpcode(1, "INC BC");
   ListeOpcodes[0x04] = FillStructOpcode(1, "INC B");
   ListeOpcodes[0x05] = FillStructOpcode(1, "DEC B");
   ListeOpcodes[0x06] = FillStructOpcode(2, "LD B, %n");
   ListeOpcodes[0x07] = FillStructOpcode(1, "RLCA");
   ListeOpcodes[0x08] = FillStructOpcode(1, "EX AF AF'");
   ListeOpcodes[0x09] = FillStructOpcode(1, "ADD HL, BC");
   ListeOpcodes[0x0A] = FillStructOpcode(1, "LD A, (BC)");
   ListeOpcodes[0x0B] = FillStructOpcode(1, "DEC BC");
   ListeOpcodes[0x0C] = FillStructOpcode(1, "INC C");
   ListeOpcodes[0x0D] = FillStructOpcode(1, "DEC C");
   ListeOpcodes[0x0E] = FillStructOpcode(2, "LD C,  %n");
   ListeOpcodes[0x0F] = FillStructOpcode(1, "RRCA");
   ListeOpcodes[0x10] = FillStructOpcode(2, "DJNZ %j__");
   ListeOpcodes[0x11] = FillStructOpcode(3, "LD DE, %nn__");
   ListeOpcodes[0x12] = FillStructOpcode(1, "LD (DE), A");
   ListeOpcodes[0x13] = FillStructOpcode(1, "INC DE");
   ListeOpcodes[0x14] = FillStructOpcode(1, "INC D");
   ListeOpcodes[0x15] = FillStructOpcode(1, "DEC D");
   ListeOpcodes[0x16] = FillStructOpcode(2, "LD D,  %n");
   ListeOpcodes[0x17] = FillStructOpcode(1, "RLA");
   ListeOpcodes[0x18] = FillStructOpcode(2, "JR %j__");
   ListeOpcodes[0x19] = FillStructOpcode(1, "ADD HL, DE");
   ListeOpcodes[0x1A] = FillStructOpcode(1, "LD A, (DE)");
   ListeOpcodes[0x1B] = FillStructOpcode(1, "DEC DE");
   ListeOpcodes[0x1C] = FillStructOpcode(1, "INC E");
   ListeOpcodes[0x1D] = FillStructOpcode(1, "DEC E");
   ListeOpcodes[0x1E] = FillStructOpcode(2, "LD E,  %n");
   ListeOpcodes[0x1F] = FillStructOpcode(1, "RRA");
   ListeOpcodes[0x20] = FillStructOpcode(2, "JR NZ  %j__");
   ListeOpcodes[0x21] = FillStructOpcode(3, "LD HL, %nn__");
   ListeOpcodes[0x22] = FillStructOpcode(3, "LD (%nn__), HL");
   ListeOpcodes[0x23] = FillStructOpcode(1, "INC HL");
   ListeOpcodes[0x24] = FillStructOpcode(1, "INC H");
   ListeOpcodes[0x25] = FillStructOpcode(1, "DEC H");
   ListeOpcodes[0x26] = FillStructOpcode(2, "LD H,  %n");
   ListeOpcodes[0x27] = FillStructOpcode(1, "DAA");
   ListeOpcodes[0x28] = FillStructOpcode(2, "JR Z  %j__");
   ListeOpcodes[0x29] = FillStructOpcode(1, "ADD HL, HL");
   ListeOpcodes[0x2A] = FillStructOpcode(3, "LD HL, (%nn__)");
   ListeOpcodes[0x2B] = FillStructOpcode(1, "DEC HL");
   ListeOpcodes[0x2C] = FillStructOpcode(1, "INC L");
   ListeOpcodes[0x2D] = FillStructOpcode(1, "DEC L");
   ListeOpcodes[0x2E] = FillStructOpcode(2, "LD L,  %n");
   ListeOpcodes[0x2F] = FillStructOpcode(1, "CPL");
   ListeOpcodes[0x30] = FillStructOpcode(2, "JR NC  %j__");
   ListeOpcodes[0x31] = FillStructOpcode(3, "LD SP, %nn__");
   ListeOpcodes[0x32] = FillStructOpcode(3, "LD (%nn__), A");
   ListeOpcodes[0x33] = FillStructOpcode(1, "INC SP");
   ListeOpcodes[0x34] = FillStructOpcode(1, "INC (HL)");
   ListeOpcodes[0x35] = FillStructOpcode(1, "DEC (HL)");
   ListeOpcodes[0x36] = FillStructOpcode(2, "LD (HL), %n");
   ListeOpcodes[0x37] = FillStructOpcode(1, "SCF");
   ListeOpcodes[0x38] = FillStructOpcode(2, "JR C, %j__");
   ListeOpcodes[0x39] = FillStructOpcode(1, "ADD HL, SP");
   ListeOpcodes[0x3A] = FillStructOpcode(3, "LD A, (%nn__)");
   ListeOpcodes[0x3B] = FillStructOpcode(1, "DEC SP");
   ListeOpcodes[0x3C] = FillStructOpcode(1, "INC A");
   ListeOpcodes[0x3D] = FillStructOpcode(1, "DEC A");
   ListeOpcodes[0x3E] = FillStructOpcode(2, "LD A, %n");
   ListeOpcodes[0x3F] = FillStructOpcode(1, "CCF");
   ListeOpcodes[0x40] = FillStructOpcode(1, "LD B, B");
   ListeOpcodes[0x41] = FillStructOpcode(1, "LD B, C");
   ListeOpcodes[0x42] = FillStructOpcode(1, "LD B, D");
   ListeOpcodes[0x43] = FillStructOpcode(1, "LD B, E");
   ListeOpcodes[0x44] = FillStructOpcode(1, "LD B, H");
   ListeOpcodes[0x45] = FillStructOpcode(1, "LD B, L");
   ListeOpcodes[0x46] = FillStructOpcode(1, "LD B, (HL)");
   ListeOpcodes[0x47] = FillStructOpcode(1, "LD B, A");
   ListeOpcodes[0x48] = FillStructOpcode(1, "LD C, B");
   ListeOpcodes[0x49] = FillStructOpcode(1, "LD C, C");
   ListeOpcodes[0x4A] = FillStructOpcode(1, "LD C, D");
   ListeOpcodes[0x4B] = FillStructOpcode(1, "LD C, E");
   ListeOpcodes[0x4C] = FillStructOpcode(1, "LD C, H");
   ListeOpcodes[0x4D] = FillStructOpcode(1, "LD C, L");
   ListeOpcodes[0x4E] = FillStructOpcode(1, "LD C, (HL)");
   ListeOpcodes[0x4F] = FillStructOpcode(1, "LD C, A");
   ListeOpcodes[0x50] = FillStructOpcode(1, "LD D, B");
   ListeOpcodes[0x51] = FillStructOpcode(1, "LD D, C");
   ListeOpcodes[0x52] = FillStructOpcode(1, "LD D, D");
   ListeOpcodes[0x53] = FillStructOpcode(1, "LD D, E");
   ListeOpcodes[0x54] = FillStructOpcode(1, "LD D, H");
   ListeOpcodes[0x55] = FillStructOpcode(1, "LD D, L");
   ListeOpcodes[0x56] = FillStructOpcode(1, "LD D, (HL)");
   ListeOpcodes[0x57] = FillStructOpcode(1, "LD D, A");
   ListeOpcodes[0x58] = FillStructOpcode(1, "LD E, B");
   ListeOpcodes[0x59] = FillStructOpcode(1, "LD E, C");
   ListeOpcodes[0x5A] = FillStructOpcode(1, "LD E, D");
   ListeOpcodes[0x5B] = FillStructOpcode(1, "LD E, E");
   ListeOpcodes[0x5C] = FillStructOpcode(1, "LD E, H");
   ListeOpcodes[0x5D] = FillStructOpcode(1, "LD E, L");
   ListeOpcodes[0x5E] = FillStructOpcode(1, "LD E, (HL)");
   ListeOpcodes[0x5F] = FillStructOpcode(1, "LD E, A");
   ListeOpcodes[0x60] = FillStructOpcode(1, "LD H, B");
   ListeOpcodes[0x61] = FillStructOpcode(1, "LD H, C");
   ListeOpcodes[0x62] = FillStructOpcode(1, "LD H, D");
   ListeOpcodes[0x63] = FillStructOpcode(1, "LD H, E");
   ListeOpcodes[0x64] = FillStructOpcode(1, "LD H, H");
   ListeOpcodes[0x65] = FillStructOpcode(1, "LD H, L");
   ListeOpcodes[0x66] = FillStructOpcode(1, "LD H, (HL)");
   ListeOpcodes[0x67] = FillStructOpcode(1, "LD H, A");
   ListeOpcodes[0x68] = FillStructOpcode(1, "LD L, B");
   ListeOpcodes[0x69] = FillStructOpcode(1, "LD L, C");
   ListeOpcodes[0x6A] = FillStructOpcode(1, "LD L, D");
   ListeOpcodes[0x6B] = FillStructOpcode(1, "LD L, E");
   ListeOpcodes[0x6C] = FillStructOpcode(1, "LD L, H");
   ListeOpcodes[0x6D] = FillStructOpcode(1, "LD L, L");
   ListeOpcodes[0x6E] = FillStructOpcode(1, "LD L, (HL)");
   ListeOpcodes[0x6F] = FillStructOpcode(1, "LD L, A");
   ListeOpcodes[0x70] = FillStructOpcode(1, "LD (HL), B");
   ListeOpcodes[0x71] = FillStructOpcode(1, "LD (HL), C");
   ListeOpcodes[0x72] = FillStructOpcode(1, "LD (HL), D");
   ListeOpcodes[0x73] = FillStructOpcode(1, "LD (HL), E");
   ListeOpcodes[0x74] = FillStructOpcode(1, "LD (HL), H");
   ListeOpcodes[0x75] = FillStructOpcode(1, "LD (HL), L");
   ListeOpcodes[0x76] = FillStructOpcode(1, "HALT");
   ListeOpcodes[0x77] = FillStructOpcode(1, "LD (HL), A");
   ListeOpcodes[0x78] = FillStructOpcode(1, "LD A, B");
   ListeOpcodes[0x79] = FillStructOpcode(1, "LD A, C");
   ListeOpcodes[0x7A] = FillStructOpcode(1, "LD A, D");
   ListeOpcodes[0x7B] = FillStructOpcode(1, "LD A, E");
   ListeOpcodes[0x7C] = FillStructOpcode(1, "LD A, H");
   ListeOpcodes[0x7D] = FillStructOpcode(1, "LD A, L");
   ListeOpcodes[0x7E] = FillStructOpcode(1, "LD A, (HL)");
   ListeOpcodes[0x7F] = FillStructOpcode(1, "LD A, A");
   ListeOpcodes[0x80] = FillStructOpcode(1, "ADD A, B");
   ListeOpcodes[0x81] = FillStructOpcode(1, "ADD A, C");
   ListeOpcodes[0x82] = FillStructOpcode(1, "ADD A, D");
   ListeOpcodes[0x83] = FillStructOpcode(1, "ADD A, E");
   ListeOpcodes[0x84] = FillStructOpcode(1, "ADD A, H");
   ListeOpcodes[0x85] = FillStructOpcode(1, "ADD A, L");
   ListeOpcodes[0x86] = FillStructOpcode(1, "ADD A, (HL)");
   ListeOpcodes[0x87] = FillStructOpcode(1, "ADD A, A");
   ListeOpcodes[0x88] = FillStructOpcode(1, "ADC A, B");
   ListeOpcodes[0x89] = FillStructOpcode(1, "ADC A, C");
   ListeOpcodes[0x8A] = FillStructOpcode(1, "ADC A, D");
   ListeOpcodes[0x8B] = FillStructOpcode(1, "ADC A, E");
   ListeOpcodes[0x8C] = FillStructOpcode(1, "ADC A, H");
   ListeOpcodes[0x8D] = FillStructOpcode(1, "ADC A, L");
   ListeOpcodes[0x8E] = FillStructOpcode(1, "ADC A, (HL)");
   ListeOpcodes[0x8F] = FillStructOpcode(1, "ADC A, A");
   ListeOpcodes[0x90] = FillStructOpcode(1, "SUB A, B");
   ListeOpcodes[0x91] = FillStructOpcode(1, "SUB A, C");
   ListeOpcodes[0x92] = FillStructOpcode(1, "SUB A, D");
   ListeOpcodes[0x93] = FillStructOpcode(1, "SUB A, E");
   ListeOpcodes[0x94] = FillStructOpcode(1, "SUB A, H");
   ListeOpcodes[0x95] = FillStructOpcode(1, "SUB A, L");
   ListeOpcodes[0x96] = FillStructOpcode(1, "SUB A, (HL)");
   ListeOpcodes[0x97] = FillStructOpcode(1, "SUB A, A");
   ListeOpcodes[0x98] = FillStructOpcode(1, "SBC A, B");
   ListeOpcodes[0x99] = FillStructOpcode(1, "SBC A, C");
   ListeOpcodes[0x9A] = FillStructOpcode(1, "SBC A, D");
   ListeOpcodes[0x9B] = FillStructOpcode(1, "SBC A, E");
   ListeOpcodes[0x9C] = FillStructOpcode(1, "SBC A, H");
   ListeOpcodes[0x9D] = FillStructOpcode(1, "SBC A, L");
   ListeOpcodes[0x9E] = FillStructOpcode(1, "SBC A, (HL)");
   ListeOpcodes[0x9F] = FillStructOpcode(1, "SBC A, A");
   ListeOpcodes[0xA0] = FillStructOpcode(1, "AND B");
   ListeOpcodes[0xA1] = FillStructOpcode(1, "AND C");
   ListeOpcodes[0xA2] = FillStructOpcode(1, "AND D");
   ListeOpcodes[0xA3] = FillStructOpcode(1, "AND E");
   ListeOpcodes[0xA4] = FillStructOpcode(1, "AND H");
   ListeOpcodes[0xA5] = FillStructOpcode(1, "AND L");
   ListeOpcodes[0xA6] = FillStructOpcode(1, "AND (HL)");
   ListeOpcodes[0xA7] = FillStructOpcode(1, "AND A");
   ListeOpcodes[0xA8] = FillStructOpcode(1, "XOR B");
   ListeOpcodes[0xA9] = FillStructOpcode(1, "XOR C");
   ListeOpcodes[0xAA] = FillStructOpcode(1, "XOR D");
   ListeOpcodes[0xAB] = FillStructOpcode(1, "XOR E");
   ListeOpcodes[0xAC] = FillStructOpcode(1, "XOR H");
   ListeOpcodes[0xAD] = FillStructOpcode(1, "XOR L");
   ListeOpcodes[0xAE] = FillStructOpcode(1, "XOR (HL)");
   ListeOpcodes[0xAF] = FillStructOpcode(1, "XOR A");
   ListeOpcodes[0xB0] = FillStructOpcode(1, "OR B");
   ListeOpcodes[0xB1] = FillStructOpcode(1, "OR C");
   ListeOpcodes[0xB2] = FillStructOpcode(1, "OR D");
   ListeOpcodes[0xB3] = FillStructOpcode(1, "OR E");
   ListeOpcodes[0xB4] = FillStructOpcode(1, "OR H");
   ListeOpcodes[0xB5] = FillStructOpcode(1, "OR L");
   ListeOpcodes[0xB6] = FillStructOpcode(1, "OR (HL)");
   ListeOpcodes[0xB7] = FillStructOpcode(1, "OR A");
   ListeOpcodes[0xB8] = FillStructOpcode(1, "CP B");
   ListeOpcodes[0xB9] = FillStructOpcode(1, "CP C");
   ListeOpcodes[0xBA] = FillStructOpcode(1, "CP D");
   ListeOpcodes[0xBB] = FillStructOpcode(1, "CP E");
   ListeOpcodes[0xBC] = FillStructOpcode(1, "CP H");
   ListeOpcodes[0xBD] = FillStructOpcode(1, "CP L");
   ListeOpcodes[0xBE] = FillStructOpcode(1, "CP (HL)");
   ListeOpcodes[0xBF] = FillStructOpcode(1, "CP A");
   ListeOpcodes[0xC0] = FillStructOpcode(1, "RET NZ");
   ListeOpcodes[0xC1] = FillStructOpcode(1, "POP BC");
   ListeOpcodes[0xC2] = FillStructOpcode(3, "JP NZ %nn__");
   ListeOpcodes[0xC3] = FillStructOpcode(3, "JP %nn__");
   ListeOpcodes[0xC4] = FillStructOpcode(3, "CALL NZ %nn__");
   ListeOpcodes[0xC5] = FillStructOpcode(1, "PUSH BC");
   ListeOpcodes[0xC6] = FillStructOpcode(2, "ADD A, %n");
   ListeOpcodes[0xC7] = FillStructOpcode(1, "RST 0");
   ListeOpcodes[0xC8] = FillStructOpcode(1, "RET Z");
   ListeOpcodes[0xC9] = FillStructOpcode(1, "RET");
   ListeOpcodes[0xCA] = FillStructOpcode(3, "JP Z %nn__");
   ListeOpcodes[0xCB] = FillStructOpcode(1, "%CB");
   ListeOpcodes[0xCC] = FillStructOpcode(3, "CALL Z %nn__");
   ListeOpcodes[0xCD] = FillStructOpcode(3, "CALL %nn__");
   ListeOpcodes[0xCE] = FillStructOpcode(2, "ADC A, %n");
   ListeOpcodes[0xCF] = FillStructOpcode(1, "RST 08H");
   ListeOpcodes[0xD0] = FillStructOpcode(1, "RET NC");
   ListeOpcodes[0xD1] = FillStructOpcode(1, "POP DE");
   ListeOpcodes[0xD2] = FillStructOpcode(3, "JP NC %nn__");
   ListeOpcodes[0xD3] = FillStructOpcode(2, "OUT (%n), A");
   ListeOpcodes[0xD4] = FillStructOpcode(3, "CALL NC %nn__");
   ListeOpcodes[0xD5] = FillStructOpcode(1, "PUSH DE");
   ListeOpcodes[0xD6] = FillStructOpcode(2, "SUB A, %n");
   ListeOpcodes[0xD7] = FillStructOpcode(1, "RST 10H");
   ListeOpcodes[0xD8] = FillStructOpcode(1, "RET C");
   ListeOpcodes[0xD9] = FillStructOpcode(1, "EXX");
   ListeOpcodes[0xDA] = FillStructOpcode(3, "JP C %nn__");
   ListeOpcodes[0xDB] = FillStructOpcode(2, "IN A, (%n)");
   ListeOpcodes[0xDC] = FillStructOpcode(3, "CALL C %nn__");
   ListeOpcodes[0xDD] = FillStructOpcode(1, "%DD");
   ListeOpcodes[0xDE] = FillStructOpcode(2, "SBC A, %n");
   ListeOpcodes[0xDF] = FillStructOpcode(1, "RST 18H");
   ListeOpcodes[0xE0] = FillStructOpcode(1, "RET PO");
   ListeOpcodes[0xE1] = FillStructOpcode(1, "POP HL");
   ListeOpcodes[0xE2] = FillStructOpcode(3, "JP PO %nn__");
   ListeOpcodes[0xE3] = FillStructOpcode(1, "EX (SP), HL");
   ListeOpcodes[0xE4] = FillStructOpcode(3, "CALL PO %nn__");
   ListeOpcodes[0xE5] = FillStructOpcode(1, "PUSH HL");
   ListeOpcodes[0xE6] = FillStructOpcode(2, "AND %n");
   ListeOpcodes[0xE7] = FillStructOpcode(1, "RST 20H");
   ListeOpcodes[0xE8] = FillStructOpcode(1, "RET PE");
   ListeOpcodes[0xE9] = FillStructOpcode(1, "JP (HL)");
   ListeOpcodes[0xEA] = FillStructOpcode(3, "JP PE %nn__");
   ListeOpcodes[0xEB] = FillStructOpcode(1, "EX DE,HL");
   ListeOpcodes[0xEC] = FillStructOpcode(3, "CALL PE %nn__");
   ListeOpcodes[0xED] = FillStructOpcode(1, "%ED");
   ListeOpcodes[0xEE] = FillStructOpcode(2, "XOR %n");
   ListeOpcodes[0xEF] = FillStructOpcode(1, "RST 28H");
   ListeOpcodes[0xF0] = FillStructOpcode(1, "RET P");
   ListeOpcodes[0xF1] = FillStructOpcode(1, "POP AF");
   ListeOpcodes[0xF2] = FillStructOpcode(3, "JP P %nn__");
   ListeOpcodes[0xF3] = FillStructOpcode(1, "DI");
   ListeOpcodes[0xF4] = FillStructOpcode(3, "CALL P %nn__");
   ListeOpcodes[0xF5] = FillStructOpcode(1, "PUSH AF");
   ListeOpcodes[0xF6] = FillStructOpcode(2, "OR %n");
   ListeOpcodes[0xF7] = FillStructOpcode(1, "RST 30H");
   ListeOpcodes[0xF8] = FillStructOpcode(1, "RET M");
   ListeOpcodes[0xF9] = FillStructOpcode(1, "LD SP, HL");
   ListeOpcodes[0xFA] = FillStructOpcode(3, "JP M %nn__");
   ListeOpcodes[0xFB] = FillStructOpcode(1, "EI");
   ListeOpcodes[0xFC] = FillStructOpcode(3, "CALL M %nn__");
   ListeOpcodes[0xFD] = FillStructOpcode(1, "%FD");
   ListeOpcodes[0xFE] = FillStructOpcode(2, "CP %n");
   ListeOpcodes[0xFF] = FillStructOpcode(1, "RST 38H");

   // Opcode a multiple byte
   // CB
   unsigned int j;
   for (j = 0x00; j <= 0x07; j++) ListeOpcodesCB[j] = FillStructOpcode(1, "RLC %r");
   for (j = 0x08; j <= 0x0F; j++) ListeOpcodesCB[j] = FillStructOpcode(1, "RRC %r");
   for (j = 0x10; j <= 0x17; j++) ListeOpcodesCB[j] = FillStructOpcode(1, "RL %r");
   for (j = 0x18; j <= 0x1F; j++) ListeOpcodesCB[j] = FillStructOpcode(1, "RR %r");
   for (j = 0x20; j <= 0x27; j++) ListeOpcodesCB[j] = FillStructOpcode(1, "SLA %r");
   for (j = 0x28; j <= 0x2F; j++) ListeOpcodesCB[j] = FillStructOpcode(1, "SRA %r");
   for (j = 0x30; j <= 0x37; j++) ListeOpcodesCB[j] = FillStructOpcode(1, "SLL %r");
   for (j = 0x38; j <= 0x3F; j++) ListeOpcodesCB[j] = FillStructOpcode(1, "SRL %r");
   DEF_OP_BIT(A); DEF_OP_BIT(B); DEF_OP_BIT(C); DEF_OP_BIT(D); DEF_OP_BIT(E); DEF_OP_BIT(H); DEF_OP_BIT(L); DEF_OP_BIT(HL);

   for (j = 0x80; j <= 0xBF; j++) ListeOpcodesCB[j] = FillStructOpcode(1, "RES %b,%r");
   for (j = 0xC0; j <= 0xFF; j++) ListeOpcodesCB[j] = FillStructOpcode(1, "SET %b,%r");

   // ED
   for (j = 0x00; j <= 0xFF; j++) ListeOpcodesED[j] = FillStructOpcode(1, "Default ED");

   ListeOpcodesED[0x40] = FillStructOpcode(1, "IN B, (C)");
   ListeOpcodesED[0x41] = FillStructOpcode(1, "OUT (C), B");
   ListeOpcodesED[0x42] = FillStructOpcode(1, "SBL HL, BC");
   ListeOpcodesED[0x43] = FillStructOpcode(3, "LD (%nn__), BC");
   ListeOpcodesED[0x44] = FillStructOpcode(1, "NEG");
   ListeOpcodesED[0x45] = FillStructOpcode(1, "RETN");
   ListeOpcodesED[0x46] = FillStructOpcode(1, "IM 0");
   ListeOpcodesED[0x47] = FillStructOpcode(1, "LD I, A");
   ListeOpcodesED[0x48] = FillStructOpcode(1, "IN C, (C)");
   ListeOpcodesED[0x49] = FillStructOpcode(1, "OUT (C), C");
   ListeOpcodesED[0x4A] = FillStructOpcode(1, "ADC HL, BC");
   ListeOpcodesED[0x4B] = FillStructOpcode(3, "LD BC, (%nn__)");
   ListeOpcodesED[0x4C] = FillStructOpcode(1, "NEG");
   ListeOpcodesED[0x4D] = FillStructOpcode(1, "RETI");
   ListeOpcodesED[0x4E] = FillStructOpcode(1, "IM 0");
   ListeOpcodesED[0x4F] = FillStructOpcode(1, "LD R, A");
   ListeOpcodesED[0x50] = FillStructOpcode(1, "IN D, (C)");
   ListeOpcodesED[0x51] = FillStructOpcode(1, "OUT (C), D");
   ListeOpcodesED[0x52] = FillStructOpcode(1, "SBL HL, DE");
   ListeOpcodesED[0x53] = FillStructOpcode(3, "LD (%nn__), DE");
   ListeOpcodesED[0x54] = FillStructOpcode(1, "NEG");
   ListeOpcodesED[0x55] = FillStructOpcode(1, "RETN");
   ListeOpcodesED[0x56] = FillStructOpcode(1, "IM 1");
   ListeOpcodesED[0x57] = FillStructOpcode(1, "LD A, I");
   ListeOpcodesED[0x58] = FillStructOpcode(1, "IN E, (C)");
   ListeOpcodesED[0x59] = FillStructOpcode(1, "OUT (C), E");
   ListeOpcodesED[0x5A] = FillStructOpcode(1, "ADC HL, DE");
   ListeOpcodesED[0x5B] = FillStructOpcode(3, "LD DE, (%nn__)");
   ListeOpcodesED[0x5C] = FillStructOpcode(1, "NEG");
   ListeOpcodesED[0x5D] = FillStructOpcode(1, "RETN");
   ListeOpcodesED[0x5E] = FillStructOpcode(1, "IM2");
   ListeOpcodesED[0x5F] = FillStructOpcode(1, "LD A, R");
   ListeOpcodesED[0x60] = FillStructOpcode(1, "IN H, (C)");
   ListeOpcodesED[0x61] = FillStructOpcode(1, "OUT (C), H");
   ListeOpcodesED[0x62] = FillStructOpcode(1, "SBC HL, HL");
   ListeOpcodesED[0x63] = FillStructOpcode(3, "LN (%nn__), HL");
   ListeOpcodesED[0x64] = FillStructOpcode(1, "NEG");
   ListeOpcodesED[0x65] = FillStructOpcode(1, "RETN");
   ListeOpcodesED[0x66] = FillStructOpcode(1, "IM 0");
   ListeOpcodesED[0x67] = FillStructOpcode(1, "RRD");
   ListeOpcodesED[0x68] = FillStructOpcode(1, "IN L, (C)");
   ListeOpcodesED[0x69] = FillStructOpcode(1, "OUT (C), L");
   ListeOpcodesED[0x6A] = FillStructOpcode(1, "ADC HL, HL");
   ListeOpcodesED[0x6B] = FillStructOpcode(3, "LN HL, (%nn__)");
   ListeOpcodesED[0x6C] = FillStructOpcode(1, "NEG");
   ListeOpcodesED[0x6D] = FillStructOpcode(1, "RETN");
   ListeOpcodesED[0x6E] = FillStructOpcode(1, "IM 0");
   ListeOpcodesED[0x6F] = FillStructOpcode(1, "RLD");
   ListeOpcodesED[0x70] = FillStructOpcode(1, "IN (C)");
   ListeOpcodesED[0x71] = FillStructOpcode(1, "OUT (C), 0");
   ListeOpcodesED[0x72] = FillStructOpcode(1, "SBC HL, SP");
   ListeOpcodesED[0x73] = FillStructOpcode(3, "LD (%nn__), SP");
   ListeOpcodesED[0x74] = FillStructOpcode(1, "NEG");
   ListeOpcodesED[0x75] = FillStructOpcode(1, "RETN");
   ListeOpcodesED[0x76] = FillStructOpcode(1, "IM 1");
   ListeOpcodesED[0x77] = FillStructOpcode(1, "NOP");
   ListeOpcodesED[0x78] = FillStructOpcode(1, "IN A, (C)");
   ListeOpcodesED[0x79] = FillStructOpcode(1, "OUT (C), A");
   ListeOpcodesED[0x7A] = FillStructOpcode(1, "ADC HL, SP");
   ListeOpcodesED[0x7B] = FillStructOpcode(3, "LD SP, (%nn__)");
   ListeOpcodesED[0x7C] = FillStructOpcode(1, "NEG");
   ListeOpcodesED[0x7D] = FillStructOpcode(1, "RETN");
   ListeOpcodesED[0x7E] = FillStructOpcode(1, "IM 2");
   ListeOpcodesED[0x7F] = FillStructOpcode(1, "NOP");
   ListeOpcodesED[0xA0] = FillStructOpcode(1, "LDI");
   ListeOpcodesED[0xA1] = FillStructOpcode(1, "CPI");
   ListeOpcodesED[0xA2] = FillStructOpcode(1, "INI");
   ListeOpcodesED[0xA3] = FillStructOpcode(1, "OUTI");
   ListeOpcodesED[0xA8] = FillStructOpcode(1, "LDD");
   ListeOpcodesED[0xA9] = FillStructOpcode(1, "CPD");
   ListeOpcodesED[0xAA] = FillStructOpcode(1, "IND");
   ListeOpcodesED[0xAB] = FillStructOpcode(1, "OUTD");
   ListeOpcodesED[0xB0] = FillStructOpcode(1, "LDIR");
   ListeOpcodesED[0xB1] = FillStructOpcode(1, "CPIR");
   ListeOpcodesED[0xB2] = FillStructOpcode(1, "INIR");
   ListeOpcodesED[0xB3] = FillStructOpcode(1, "OTIR");
   ListeOpcodesED[0xB8] = FillStructOpcode(1, "LDDR");
   ListeOpcodesED[0xB9] = FillStructOpcode(1, "CPDR");
   ListeOpcodesED[0xBA] = FillStructOpcode(1, "INDR");
   ListeOpcodesED[0xBB] = FillStructOpcode(1, "OTDR");
   ListeOpcodesED[0xED] = FillStructOpcode(1, "%ED");

   // DD
   // Act like normal opcode, except that HL is replaced by IX.
   for (j = 0x00; j <= 0xff; j++)
   {
      ListeOpcodesDD[j] = ListeOpcodes[j];
   }

   ListeOpcodesDD[0x09] = FillStructOpcode(1, "ADD IX, BC");
   ListeOpcodesDD[0x19] = FillStructOpcode(1, "ADD IX, DE");
   ListeOpcodesDD[0x21] = FillStructOpcode(3, "LD IX, %nn__");
   ListeOpcodesDD[0x22] = FillStructOpcode(3, "LD (%nn__), IX");
   ListeOpcodesDD[0x23] = FillStructOpcode(1, "INC IX");
   ListeOpcodesDD[0x24] = FillStructOpcode(1, "INC IXh");
   ListeOpcodesDD[0x25] = FillStructOpcode(1, "DEC IXh");
   ListeOpcodesDD[0x26] = FillStructOpcode(2, "LD IXh, %n");
   ListeOpcodesDD[0x29] = FillStructOpcode(1, "ADD IX, IX");
   ListeOpcodesDD[0x2A] = FillStructOpcode(3, "LD IX, (%nn__)");
   ListeOpcodesDD[0x2B] = FillStructOpcode(1, "DEC IX");
   ListeOpcodesDD[0x2C] = FillStructOpcode(1, "INC IXl");
   ListeOpcodesDD[0x2D] = FillStructOpcode(1, "DEC IXl");
   ListeOpcodesDD[0x2E] = FillStructOpcode(2, "LD IXl, %n");
   ListeOpcodesDD[0x34] = FillStructOpcode(1, "INC (IX+%n)");
   ListeOpcodesDD[0x35] = FillStructOpcode(1, "DEC (IX+%n)");
   ListeOpcodesDD[0x36] = FillStructOpcode(3, "LD (IX+%n2), %n");
   ListeOpcodesDD[0x39] = FillStructOpcode(1, "ADD IX, SP");
   ListeOpcodesDD[0x44] = FillStructOpcode(2, "LD B, IXh");
   ListeOpcodesDD[0x45] = FillStructOpcode(2, "LD B, IXl");
   ListeOpcodesDD[0x46] = FillStructOpcode(2, "LD B, (IX+%n)");
   ListeOpcodesDD[0x4C] = FillStructOpcode(2, "LD C, IXh");
   ListeOpcodesDD[0x4D] = FillStructOpcode(2, "LD C, IXl");
   ListeOpcodesDD[0x4E] = FillStructOpcode(2, "LD C, (IX+%n)");
   ListeOpcodesDD[0x54] = FillStructOpcode(2, "LD D, IXh");
   ListeOpcodesDD[0x55] = FillStructOpcode(2, "LD D, IXl");
   ListeOpcodesDD[0x56] = FillStructOpcode(2, "LD D, (IX+%n)");
   ListeOpcodesDD[0x5C] = FillStructOpcode(2, "LD E, IXh");
   ListeOpcodesDD[0x5D] = FillStructOpcode(2, "LD E, IXl");
   ListeOpcodesDD[0x5E] = FillStructOpcode(2, "LD E, (IX+%n)");
   ListeOpcodesDD[0x60] = FillStructOpcode(2, "LD IXh, B");
   ListeOpcodesDD[0x61] = FillStructOpcode(2, "LD IXh, C");
   ListeOpcodesDD[0x62] = FillStructOpcode(2, "LD IXh, D");
   ListeOpcodesDD[0x63] = FillStructOpcode(2, "LD IXh, E");
   ListeOpcodesDD[0x64] = FillStructOpcode(2, "LD IXh, IXh");
   ListeOpcodesDD[0x65] = FillStructOpcode(2, "LD IXh, IXl");
   ListeOpcodesDD[0x66] = FillStructOpcode(2, "LD H, (IX+%n)");
   ListeOpcodesDD[0x67] = FillStructOpcode(2, "LD IXh, A");
   ListeOpcodesDD[0x68] = FillStructOpcode(2, "LD IXl, B");
   ListeOpcodesDD[0x69] = FillStructOpcode(2, "LD IXl, C");
   ListeOpcodesDD[0x6A] = FillStructOpcode(2, "LD IXl, D");
   ListeOpcodesDD[0x6B] = FillStructOpcode(2, "LD IXl, E");
   ListeOpcodesDD[0x6C] = FillStructOpcode(2, "LD IXl, IXh");
   ListeOpcodesDD[0x6D] = FillStructOpcode(2, "LD IXl, IXl");
   ListeOpcodesDD[0x6E] = FillStructOpcode(2, "LD L, (IX+%n)");
   ListeOpcodesDD[0x6F] = FillStructOpcode(2, "LD IXh, A");
   ListeOpcodesDD[0x70] = FillStructOpcode(2, "LD (IX+%n), B");
   ListeOpcodesDD[0x71] = FillStructOpcode(2, "LD (IX+%n), C");
   ListeOpcodesDD[0x72] = FillStructOpcode(2, "LD (IX+%n), D");
   ListeOpcodesDD[0x73] = FillStructOpcode(2, "LD (IX+%n), E");
   ListeOpcodesDD[0x74] = FillStructOpcode(2, "LD (IX+%n), H");
   ListeOpcodesDD[0x75] = FillStructOpcode(2, "LD (IX+%n), L");
   ListeOpcodesDD[0x77] = FillStructOpcode(2, "LD (IX+%n), A");
   ListeOpcodesDD[0x7C] = FillStructOpcode(2, "LD A, IXh");
   ListeOpcodesDD[0x7D] = FillStructOpcode(1, "LD A, IXl");
   ListeOpcodesDD[0x7E] = FillStructOpcode(2, "LD A, (IX+%n)");
   ListeOpcodesDD[0x84] = FillStructOpcode(2, "ADD A, IXh");
   ListeOpcodesDD[0x85] = FillStructOpcode(2, "ADD A, IXl");
   ListeOpcodesDD[0x86] = FillStructOpcode(2, "ADD A, (IX+%n)");
   ListeOpcodesDD[0x8C] = FillStructOpcode(2, "ADC A, IXh");
   ListeOpcodesDD[0x8D] = FillStructOpcode(2, "ADC A, IXl");
   ListeOpcodesDD[0x8E] = FillStructOpcode(2, "ADC A, (IX+%n)");
   ListeOpcodesDD[0x94] = FillStructOpcode(2, "SUB A, IXh");
   ListeOpcodesDD[0x95] = FillStructOpcode(2, "SUB A, IXl");
   ListeOpcodesDD[0x96] = FillStructOpcode(2, "SUB A, (IX+%n)");
   ListeOpcodesDD[0x9C] = FillStructOpcode(2, "SBC A, IXh");
   ListeOpcodesDD[0x9D] = FillStructOpcode(2, "SBC A, IXl");
   ListeOpcodesDD[0x9E] = FillStructOpcode(2, "SBC A, (IX+%n)");
   ListeOpcodesDD[0xA4] = FillStructOpcode(2, "AND A, IXh");
   ListeOpcodesDD[0xA5] = FillStructOpcode(2, "AND A, IXl");
   ListeOpcodesDD[0xA6] = FillStructOpcode(2, "AND (IX+%n)");
   ListeOpcodesDD[0xAC] = FillStructOpcode(2, "XOR A, IXh");
   ListeOpcodesDD[0xAD] = FillStructOpcode(2, "XOR A, IXl");
   ListeOpcodesDD[0xAE] = FillStructOpcode(2, "XOR (IX+%n)");
   ListeOpcodesDD[0xB4] = FillStructOpcode(2, "OR A, IXh");
   ListeOpcodesDD[0xB5] = FillStructOpcode(2, "OR A, IXl");
   ListeOpcodesDD[0xB6] = FillStructOpcode(2, "OR (IX+%n)");
   ListeOpcodesDD[0xBC] = FillStructOpcode(2, "CP IXh");
   ListeOpcodesDD[0xBD] = FillStructOpcode(2, "CP (IXl");
   ListeOpcodesDD[0xBE] = FillStructOpcode(2, "CP (IX+%n)");
   ListeOpcodesDD[0xCB] = FillStructOpcode(4, "RES b, (IX+%n)");
   ListeOpcodesDD[0xE1] = FillStructOpcode(1, "POP IX");
   ListeOpcodesDD[0xE3] = FillStructOpcode(1, "EX SP, IX");
   ListeOpcodesDD[0xE5] = FillStructOpcode(1, "PUSH IX");
   ListeOpcodesDD[0xE9] = FillStructOpcode(1, "JP (IX)");
   ListeOpcodesDD[0xF9] = FillStructOpcode(1, "LD SP, IX");

   // FD
   // Act like normal opcode, except that HL is replaced by IY.
   for (j = 0x00; j <= 0xff; j++)
   {
      ListeOpcodesFD[j] = ListeOpcodes[j];
   }

   ListeOpcodesFD[0x09] = FillStructOpcode(1, "ADD IY, BC");
   ListeOpcodesFD[0x19] = FillStructOpcode(1, "ADD IY, DE");
   ListeOpcodesFD[0x21] = FillStructOpcode(3, "LD IY, %nn__");
   ListeOpcodesFD[0x22] = FillStructOpcode(3, "LD (%nn__), IY");
   ListeOpcodesFD[0x23] = FillStructOpcode(1, "INC IY");
   ListeOpcodesFD[0x24] = FillStructOpcode(1, "INC IYh");
   ListeOpcodesFD[0x25] = FillStructOpcode(1, "DEC IYh");
   ListeOpcodesFD[0x26] = FillStructOpcode(2, "LD IYh, %n");
   ListeOpcodesFD[0x29] = FillStructOpcode(1, "ADD IY, IY");
   ListeOpcodesFD[0x2A] = FillStructOpcode(3, "LD IY, (%nn__)");
   ListeOpcodesFD[0x2B] = FillStructOpcode(1, "DEC IY");
   ListeOpcodesFD[0x2C] = FillStructOpcode(1, "INC IYl");
   ListeOpcodesFD[0x2D] = FillStructOpcode(1, "DEC IYl");
   ListeOpcodesFD[0x2E] = FillStructOpcode(2, "LD IYl, %n");
   ListeOpcodesFD[0x34] = FillStructOpcode(2, "INC (IY+%n)");
   ListeOpcodesFD[0x35] = FillStructOpcode(2, "DEC (IY+%n)");
   ListeOpcodesFD[0x36] = FillStructOpcode(3, "LD (IY+%n2), %n");
   ListeOpcodesFD[0x39] = FillStructOpcode(1, "ADD IY, SP");
   ListeOpcodesFD[0x44] = FillStructOpcode(2, "LD B, IYh");
   ListeOpcodesFD[0x45] = FillStructOpcode(2, "LD B, IYl");
   ListeOpcodesFD[0x46] = FillStructOpcode(2, "LD B, (IY+%n)");
   ListeOpcodesFD[0x4C] = FillStructOpcode(2, "LD C, IYh");
   ListeOpcodesFD[0x4D] = FillStructOpcode(2, "LD C, IYl");
   ListeOpcodesFD[0x4E] = FillStructOpcode(2, "LD C, (IY+%n)");
   ListeOpcodesFD[0x54] = FillStructOpcode(2, "LD D, IYh");
   ListeOpcodesFD[0x55] = FillStructOpcode(2, "LD D, IYl");
   ListeOpcodesFD[0x56] = FillStructOpcode(2, "LD D, (IY+%n)");
   ListeOpcodesFD[0x5C] = FillStructOpcode(2, "LD E, IYh");
   ListeOpcodesFD[0x5D] = FillStructOpcode(2, "LD E, IYl");
   ListeOpcodesFD[0x5E] = FillStructOpcode(2, "LD E, (IY+%n)");
   ListeOpcodesFD[0x60] = FillStructOpcode(2, "LD IYh, B");
   ListeOpcodesFD[0x61] = FillStructOpcode(2, "LD IYh, C");
   ListeOpcodesFD[0x62] = FillStructOpcode(2, "LD IYh, D");
   ListeOpcodesFD[0x63] = FillStructOpcode(2, "LD IYh, E");
   ListeOpcodesFD[0x64] = FillStructOpcode(2, "LD IYh, IYh");
   ListeOpcodesFD[0x65] = FillStructOpcode(2, "LD IYh, IYl");
   ListeOpcodesFD[0x66] = FillStructOpcode(2, "LD H, (IY+%n)");
   ListeOpcodesFD[0x67] = FillStructOpcode(2, "LD IYh, A");
   ListeOpcodesFD[0x68] = FillStructOpcode(2, "LD IYh, B");
   ListeOpcodesFD[0x69] = FillStructOpcode(2, "LD IYh, C");
   ListeOpcodesFD[0x6A] = FillStructOpcode(2, "LD IYh, D");
   ListeOpcodesFD[0x6B] = FillStructOpcode(2, "LD IYh, E");
   ListeOpcodesFD[0x6C] = FillStructOpcode(2, "LD IYh, IYh");
   ListeOpcodesFD[0x6D] = FillStructOpcode(2, "LD IYh, IYl");
   ListeOpcodesFD[0x6E] = FillStructOpcode(2, "LD L, (IY+%n)");
   ListeOpcodesFD[0x6F] = FillStructOpcode(2, "LD IYh, A");
   ListeOpcodesFD[0x70] = FillStructOpcode(2, "LD (IY+%n), B");
   ListeOpcodesFD[0x71] = FillStructOpcode(2, "LD (IY+%n), C");
   ListeOpcodesFD[0x72] = FillStructOpcode(2, "LD (IY+%n), D");
   ListeOpcodesFD[0x73] = FillStructOpcode(2, "LD (IY+%n), E");
   ListeOpcodesFD[0x74] = FillStructOpcode(2, "LD (IY+%n), H");
   ListeOpcodesFD[0x75] = FillStructOpcode(2, "LD (IY+%n), L");
   ListeOpcodesFD[0x77] = FillStructOpcode(2, "LD (IY+%n), A");
   ListeOpcodesFD[0x7C] = FillStructOpcode(2, "LD A, IYh");
   ListeOpcodesFD[0x7D] = FillStructOpcode(1, "LD A, IYl");
   ListeOpcodesFD[0x7E] = FillStructOpcode(2, "LD A, (IY+%n)");
   ListeOpcodesFD[0x84] = FillStructOpcode(2, "ADD A, IYh");
   ListeOpcodesFD[0x85] = FillStructOpcode(2, "ADD A, IYl");
   ListeOpcodesFD[0x86] = FillStructOpcode(2, "ADD A, (IY+%n)");
   ListeOpcodesFD[0x8C] = FillStructOpcode(2, "ADC A, IYh");
   ListeOpcodesFD[0x8D] = FillStructOpcode(2, "ADC A, IYl");
   ListeOpcodesFD[0x8E] = FillStructOpcode(2, "ADC A, (IY+%n)");
   ListeOpcodesFD[0x94] = FillStructOpcode(2, "SUB A, IYh");
   ListeOpcodesFD[0x95] = FillStructOpcode(2, "SUB A, IYl");
   ListeOpcodesFD[0x96] = FillStructOpcode(2, "SUB A, (IY+%n)");
   ListeOpcodesFD[0x9C] = FillStructOpcode(2, "SBC A, IYh");
   ListeOpcodesFD[0x9D] = FillStructOpcode(2, "SBC A, IYl");
   ListeOpcodesFD[0x9E] = FillStructOpcode(2, "SBC A, (IY+%n)");
   ListeOpcodesFD[0xA4] = FillStructOpcode(2, "AND A, IYh");
   ListeOpcodesFD[0xA5] = FillStructOpcode(2, "AND A, IYl");
   ListeOpcodesFD[0xA6] = FillStructOpcode(2, "AND (IY+%n)");
   ListeOpcodesFD[0xAC] = FillStructOpcode(2, "XOR A, IYh");
   ListeOpcodesFD[0xAD] = FillStructOpcode(2, "XOR A, IYl");
   ListeOpcodesFD[0xAE] = FillStructOpcode(2, "XOR (IY+%n)");
   ListeOpcodesFD[0xB4] = FillStructOpcode(2, "OR A, IYh");
   ListeOpcodesFD[0xB5] = FillStructOpcode(2, "OR A, IYl");
   ListeOpcodesFD[0xB6] = FillStructOpcode(2, "OR (IY+%n)");
   ListeOpcodesFD[0xBC] = FillStructOpcode(2, "CP IYh");
   ListeOpcodesFD[0xBD] = FillStructOpcode(2, "CP (IYl");
   ListeOpcodesFD[0xBE] = FillStructOpcode(2, "CP (IY+%n)");
   ListeOpcodesFD[0xCB] = FillStructOpcode(3, "RES %b, (IY+%n)");
   ListeOpcodesFD[0xE1] = FillStructOpcode(1, "POP_IY");
   ListeOpcodesFD[0xE3] = FillStructOpcode(1, "EX (SP), IY");
   ListeOpcodesFD[0xE5] = FillStructOpcode(1, "PUSH IY");
   ListeOpcodesFD[0xE9] = FillStructOpcode(1, "JP (IY)");
   ListeOpcodesFD[0xF9] = FillStructOpcode(1, "LD SP, IY");
}
