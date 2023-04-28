#include "MemoryViewWidget.h"

#include <QPainter>
#include <QResizeEvent>

MemoryViewWidget::MemoryViewWidget(QWidget* parent )
   : QWidget(parent),
   vertical_sb_(Qt::Vertical, this),
   horizontal_sb_(Qt::Horizontal, this),
   machine_(nullptr),
   max_address_(0xFFFF),
   current_address_(0),
   current_line_selected_(-1),
   nb_lines_(0),
   line_height_(0),
   nb_byte_per_lines_(0),
   address_size_(0),
   char_size_(0),
   margin_size_(0),
   top_margin_(0),
   back_color_(220, 220, 220),
   address_color_(Qt::blue),
   byte_color_(Qt::darkGray),
   char_color_(Qt::gray)
{
   setFocusPolicy(Qt::StrongFocus);

   connect(&vertical_sb_, SIGNAL(valueChanged(int)), this, SLOT(OnValueChange(int)));
   connect(&horizontal_sb_, SIGNAL(valueChanged(int)), this, SLOT(OnValueChangeHorizontal(int)));
}

void MemoryViewWidget::SetDisassemblyInfo(Emulation* machine, unsigned short max_address)
{
   machine_ = machine->GetEngine();
   max_address_ = max_address;
}

void MemoryViewWidget::ForceTopAddress(unsigned short address)
{
   current_address_ = address;
   // Set scroller
   vertical_sb_.setValue(current_address_);

   // Update
   repaint();
}

void MemoryViewWidget::wheelEvent(QWheelEvent* event)
{
   QPoint numPixels = event->pixelDelta();
   QPoint numDegrees = event->angleDelta()/8;

   if (!numPixels.isNull()) 
   {
      // todo
    
   }
   else if (!numDegrees.isNull()) 
   {
      current_address_ -= ( numDegrees.y() / 15) * nb_byte_per_lines_;
   }
   repaint();
   event->accept();
}

void MemoryViewWidget::keyPressEvent(QKeyEvent* event)
{
   switch (event->key())
   {
   case Qt::Key_Down:
      current_address_ += nb_byte_per_lines_;
      break;
   case Qt::Key_Up:
      current_address_ -= nb_byte_per_lines_;
      break;
   case Qt::Key_PageDown:
      current_address_ += nb_byte_per_lines_ * nb_lines_;
      break;
   case Qt::Key_PageUp:
      current_address_ -= nb_byte_per_lines_ * nb_lines_;
      break;
   default:
      // todo
      return;
   }
   event->accept();
   current_address_ &= max_address_;
   repaint();
}

void MemoryViewWidget::mousePressEvent(QMouseEvent* event)
{
   // Check line
   int line = event->y() / line_height_;

   // check position
   int x = event->x();
}

void MemoryViewWidget::paintEvent(QPaintEvent* /* event */)
{
   QPainter painter(this);
   const int width = size().width();
   const int height = size().height();

   painter.fillRect(0, 0, width, height, back_color_);

   // Draw every lines 
   unsigned short line_address = current_address_;
   char address[16];

   std::string byte_buffer;
   std::string char_buffer;
   byte_buffer.resize(nb_byte_per_lines_ * 3);
   char_buffer.resize(nb_byte_per_lines_ );

   for (int i = 0; i < nb_lines_; i++)
   {
      // Address 
      sprintf(address, "%4.4X: ", line_address);
      painter.setPen(address_color_);
      painter.drawText(margin_size_, top_margin_ + line_height_ * i, address_size_, line_height_,Qt::AlignLeft|Qt::AlignVCenter, address);

      // Bytes 
      byte_buffer.clear();
      for (int j = 0; j < nb_byte_per_lines_; j++)
      {
         char byte[4] = { 0 };
         unsigned char b = machine_->GetMem()->Get(line_address + j);
         sprintf(byte, "%2.2X ", b);
         byte_buffer += byte;

         if (b >= 0x20 && b <= 126)
         {
            char_buffer[j] = b;
         }
         else
         {
            char_buffer[j] = '.';
         }
      }
      painter.setPen(byte_color_);
      painter.drawText(margin_size_ + address_size_, top_margin_ + line_height_ * i, char_size_* nb_byte_per_lines_*3, line_height_, Qt::AlignLeft|Qt::AlignVCenter, byte_buffer.c_str());
      // Character (if displayable)
      painter.setPen(char_color_);
      painter.drawText(margin_size_ + address_size_ + char_size_* nb_byte_per_lines_*3, top_margin_ + line_height_ * i, char_size_* nb_byte_per_lines_, line_height_, Qt::AlignLeft|Qt::AlignVCenter, char_buffer.c_str());

      line_address += nb_byte_per_lines_;
   }
}

void MemoryViewWidget::resizeEvent(QResizeEvent* e)
{
   // Set scrollbar   
   vertical_sb_.move(e->size().width() - vertical_sb_.sizeHint().width(), 0);
   vertical_sb_.resize(vertical_sb_.sizeHint().width(), e->size().height() - horizontal_sb_.sizeHint().height());

   horizontal_sb_.move(0, e->size().height() - horizontal_sb_.sizeHint().height());
   horizontal_sb_.resize(e->size().width() - vertical_sb_.sizeHint().width(), horizontal_sb_.sizeHint().height());

   ComputeScrollArea();

}

void MemoryViewWidget::OnValueChange(int valueScrollBar)
{
   current_address_ = static_cast<unsigned int>(valueScrollBar);
   // Update;
   repaint();
}

void MemoryViewWidget::OnValueChangeHorizontal(int valueScrollBar)
{
   margin_size_ = valueScrollBar * -1;
   // Update;
   repaint();
}

void MemoryViewWidget::ComputeScrollArea()
{
   // Number of displayed lines
   const QFontMetrics fm (property("font").value<QFont>());

   address_size_ = fm.horizontalAdvance("FFFF: ");
   char_size_ = fm.horizontalAdvance(' ');

   line_height_ = std::max(fm.lineSpacing(), line_height_);

   nb_lines_ = (size().height() - horizontal_sb_.sizeHint().height()) / line_height_;
   nb_byte_per_lines_ = 16;

   int used_width = address_size_ + char_size_*16*4;

   // Horizontal
   if (used_width > width())
   {
      horizontal_sb_.setMaximum(used_width - width() );
      horizontal_sb_.setMinimum(0);
      horizontal_sb_.setSingleStep(1);
      horizontal_sb_.setPageStep(1);
      horizontal_sb_.setValue(0);
      horizontal_sb_.show();
      margin_size_ = 0;
   }
   else
   {
      margin_size_ = 0;
      horizontal_sb_.hide();
   }

   // Vertical:
   // Compute number of lines : 0 -> max_address_
   vertical_sb_.setMaximum(static_cast<int>((max_address_ > (nb_lines_ - 1))?(max_address_ - (nb_lines_-1)):(0)));
   vertical_sb_.setMinimum(0);
   vertical_sb_.setSingleStep(1);
   vertical_sb_.setPageStep(static_cast<int>(nb_lines_));
   vertical_sb_.setValue(static_cast<int>(current_address_));

}

