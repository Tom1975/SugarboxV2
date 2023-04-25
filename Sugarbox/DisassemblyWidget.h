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
   void wheelEvent(QWheelEvent* event);
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
   Z80Desassember* disassembler_;
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
};
