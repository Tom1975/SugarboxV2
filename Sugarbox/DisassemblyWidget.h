#pragma once

#include <QWidget>
#include <QScrollBar>

#include "Emulation.h"
#include "FlagHandler.h"
#include "SettingsValues.h"
#include "Settings.h"


class DisassemblyWidget : public QWidget
{
   Q_OBJECT

public:
   explicit DisassemblyWidget(QWidget* parent = nullptr);

   void SetDisassemblyInfo(Emulation* machine, unsigned short max_address);
   void SetFlagHandler(FlagHandler* flag_handler);
   void SetSettings(Settings* settings);

   void ForceTopAddress(unsigned short address);

protected:
   void wheelEvent(QWheelEvent* event) override;
   void paintEvent(QPaintEvent* event) override;
   void resizeEvent(QResizeEvent* e) override;
   void keyPressEvent(QKeyEvent* event) override;
   void mousePressEvent(QMouseEvent* event) override;

   unsigned short GetMaxedPreviousValidAddress(unsigned short Addr_P);
   unsigned short GetPreviousValidAddress(unsigned short Addr_P);
   void GoUp();
   void ComputeScrollArea();

public slots:
   void OnValueChange(int valueScrollBar);
   void OnValueChangeHorizontal(int valueScrollBar);

private:
   QWidget disasm_window_;
   QScrollBar vertical_sb_;
   QScrollBar horizontal_sb_;

   EmulatorEngine* machine_;
   Z80Desassember* disassembler_;
   Settings* settings_;

   FlagHandler* flag_handler_;
   unsigned short max_address_;
   unsigned short current_address_;
   int current_line_selected_;
   int nb_lines_;
   int line_height_;
   std::vector<unsigned short> line_address_;

   int margin_size_ ;
   int horizontal_offset_;
   int top_margin_;

   // Ressources
   QPixmap bp_pixmap_;
   QPixmap flag_pixmap_;
   QPixmap pc_pixmap_;

   // Display Settings
   QColor back_color_;
   QColor margin_color_;
   QColor address_color_;
   QColor mnemonic_color_;
   QColor arg_color_;
   QColor byte_color_;
   QColor char_color_;
   QColor sel_color_;
};
