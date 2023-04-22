#include "DisassemblyWidget.h"

#include <QPainter>
#include <QResizeEvent>

DisassemblyWidget::DisassemblyWidget(QWidget* parent )
   : QWidget(parent),
   disasm_window_(this),
   vertical_sb_(Qt::Vertical, this),
   horizontal_sb_(Qt::Horizontal, this),
   max_adress_(0xFFFF),
   current_address_(0)

{
   connect(&vertical_sb_, SIGNAL(valueChanged(int)), this, SLOT(OnValueChange(int)));
}

void DisassemblyWidget::paintEvent(QPaintEvent* /* event */)
{
   QPainter painter(this);
   int width = size().width() - 3;
   int height = size().height() - 5;

   painter.fillRect(0, 0, width, height, QColor(220, 220, 220));

   QString text = QString("Adress : %1").arg(current_address_);
   painter.drawText(10, 10, text);

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

void DisassemblyWidget::SetDisassemblyInfo(unsigned int max_adress)
{
   max_adress_ = max_adress;
}

void DisassemblyWidget::ComputeScrollArea()
{
   // Vertical:
   // Compute number of lines : 0 -> max_adress_
   vertical_sb_.setMaximum(max_adress_);
   vertical_sb_.setMinimum(0);
   vertical_sb_.setSingleStep(1);
   vertical_sb_.setPageStep(15); // TODO

}
