#pragma once

#include <QWidget>
#include <QScrollBar>

#include "Emulation.h"
#include "FlagHandler.h"

class MemoryViewWidget : public QWidget
{
   Q_OBJECT

public:
   explicit MemoryViewWidget(QWidget* parent = nullptr);

   void SetDisassemblyInfo(Emulation* machine, unsigned short max_address);
   void ForceTopAddress(unsigned short address);

protected:
   void wheelEvent(QWheelEvent* event);
   void paintEvent(QPaintEvent* event) override;
   void resizeEvent(QResizeEvent* e) override;
   void keyPressEvent(QKeyEvent* event) override;
   void mousePressEvent(QMouseEvent* event)override;

   void ComputeScrollArea();

public slots:
   void OnValueChange(int valueScrollBar);
   void OnValueChangeHorizontal(int valueScrollBar);
   


private:
   QScrollBar vertical_sb_;
   QScrollBar horizontal_sb_;

   EmulatorEngine* machine_;
   unsigned short max_address_;
   int current_address_;
   int current_line_selected_;
   int nb_lines_;
   int line_height_;
   int nb_byte_per_lines_;

   unsigned int address_size_;
   int char_size_;

   int margin_size_ ;
   int top_margin_;

   // Display Settings
   QColor back_color_;
   QColor address_color_;
   QColor byte_color_;
   QColor char_color_;
};
