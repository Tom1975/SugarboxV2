#include "StackViewWidget.h"

#include <QPainter>
#include <QResizeEvent>

StackViewWidget::StackViewWidget(QWidget* parent) : 
   MemoryViewWidget(parent),
   pc_pixmap_(":/Resources/PC.png")
{
   margin_size_ = pc_pixmap_.width();
}

void StackViewWidget::paintEvent(QPaintEvent* event)
{
   QPainter painter(this);
   const int width = size().width();
   const int height = size().height();

   painter.fillRect(0, 0, width, height, back_color_);

   // Draw every lines 
   unsigned short line_address = current_address_;
   char address[16];

   nb_byte_per_lines_ = 2;

   for (int i = 0; i < nb_lines_; i++)
   {
      // is this stack pointer ?
      if (machine_->GetProc()->sp_ == line_address)
      {
         painter.drawPixmap(0, top_margin_ + line_height_ * i, pc_pixmap_);
      }

      // Address 
      sprintf(address, "%4.4X: ", line_address);
      painter.setPen(address_color_);
      painter.drawText(margin_size_, top_margin_ + line_height_ * i, address_size_, line_height_, Qt::AlignLeft | Qt::AlignVCenter, address);

      char byte[5] = { 0 };
      unsigned short w = (machine_->GetMem()->Get(line_address )<<8) | (machine_->GetMem()->Get(line_address));
      sprintf(byte, "%4.4X", w);

      painter.setPen(byte_color_);
      painter.drawText(margin_size_ + address_size_, top_margin_ + line_height_ * i, char_size_ * 4, line_height_, Qt::AlignLeft | Qt::AlignVCenter, byte);

      line_address += 2;
   }
}

void StackViewWidget::resizeEvent(QResizeEvent* e)
{
   // Set scrollbar   
   vertical_sb_.move(e->size().width() - vertical_sb_.sizeHint().width(), 0);
   vertical_sb_.resize(vertical_sb_.sizeHint().width(), e->size().height() - horizontal_sb_.sizeHint().height());

   // Number of displayed lines
   const QFontMetrics fm(property("font").value<QFont>());

   address_size_ = fm.horizontalAdvance("FFFF: ");
   char_size_ = fm.horizontalAdvance(' ');

   line_height_ = std::max(fm.lineSpacing(), line_height_);

   nb_lines_ = (size().height() - horizontal_sb_.sizeHint().height()) / line_height_;
   nb_byte_per_lines_ = 2;

   int used_width = address_size_ + char_size_ * nb_byte_per_lines_ * 4;

   horizontal_sb_.hide();

   // Vertical:
   // Compute number of lines : 0 -> max_address_
   vertical_sb_.setMaximum(static_cast<int>((max_address_ > (nb_lines_ - 1)) ? (max_address_ - (nb_lines_ - 1)) : (0)));
   vertical_sb_.setMinimum(0);
   vertical_sb_.setSingleStep(1);
   vertical_sb_.setPageStep(static_cast<int>(nb_lines_));
   vertical_sb_.setValue(static_cast<int>(current_address_));

}