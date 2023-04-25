#pragma once

#include <QWidget>
#include <QScrollBar>

#include "Emulation.h"


class DisassemblyWidget : public QWidget
{
   Q_OBJECT

public:
   explicit DisassemblyWidget(QWidget* parent = nullptr);

   void SetDisassemblyInfo(Emulation* machine, unsigned short max_address);
   void ForceTopAddress(unsigned short address);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* e) override;

    void ComputeScrollArea();

    void InitOpcodeShortcuts();
    const int DasmMnemonic(unsigned short addr, char mnemonic[16], char argument[16]) const;

public slots:
    void OnValueChange(int valueScrollBar);


private:
   QWidget disasm_window_;
   QScrollBar vertical_sb_;
   QScrollBar horizontal_sb_;

   EmulatorEngine* machine_;
   unsigned short max_address_;
   unsigned int current_address_;
   unsigned int nb_lines_;
   unsigned int line_height_;
   std::vector<unsigned short> line_address_;

   unsigned int margin_size_ ;

   // Ressources
   QPixmap bp_pixmap_;
   QPixmap flag_pixmap_;
   QPixmap pc_pixmap_;

   // Disassembly values
#define MAX_MSTATE_NB 6
#define MAX_DISASSEMBLY_SIZE 32
   struct Opcode
   {
      unsigned char Size;
      char Disassembly[MAX_DISASSEMBLY_SIZE];
   };

   Opcode FillStructOpcode(unsigned char Size, char* Disassembly)
   {
      Opcode op;
      op.Size = Size;
      strcpy(op.Disassembly, Disassembly);
      return op;
   };
   Opcode ListeOpcodes[256];
   Opcode ListeOpcodesCB[256];
   Opcode ListeOpcodesED[256];
   Opcode ListeOpcodesDD[256];
   Opcode ListeOpcodesFD[256];
};
