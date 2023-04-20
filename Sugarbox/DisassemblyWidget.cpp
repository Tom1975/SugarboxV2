#include "DisassemblyWidget.h"

#include <QPainter>
#include <QResizeEvent>

DisassemblyWidget::DisassemblyWidget(QWidget* parent )
   : QWidget(parent),
   disasm_window_(this),
   vertical_sb_(Qt::Vertical, this),
   horizontal_sb_(Qt::Horizontal, this)

{
   
}

void DisassemblyWidget::paintEvent(QPaintEvent* /* event */)
{
   QPainter painter(this);
   int width = size().width() - 3;
   int height = size().height() - 5;

   painter.fillRect(0, 0, width, height, QColor(220, 220, 220));
   painter.drawText(10, 10, "TEST " );

}

void DisassemblyWidget::resizeEvent(QResizeEvent* e)
{
   // If the widget's width has changed, we recalculate the new height
   // of our widget.
   if (e->size().width() == e->oldSize().width()) {
      return;
   }

   // Do all the stuff : 
   // Set scrollbar
   vertical_sb_.move(e->size().width() - vertical_sb_.size().width(),
      e->size().height() - horizontal_sb_.size().height());

   horizontal_sb_.move(e->size().width() - vertical_sb_.size().width(),
      e->size().height() - horizontal_sb_.size().height());


}
