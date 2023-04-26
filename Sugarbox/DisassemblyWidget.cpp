#include "DisassemblyWidget.h"

#include <QPainter>
#include <QResizeEvent>

DisassemblyWidget::DisassemblyWidget(QWidget* parent )
   : QWidget(parent),
   disasm_window_(this),
   vertical_sb_(Qt::Vertical, this),
   horizontal_sb_(Qt::Horizontal, this),
   machine_(nullptr),
   disassembler_(nullptr),
   max_address_(0xFFFF),
   current_address_(0),
   nb_lines_(0),
   bp_pixmap_(":/Resources/bp.png"),
   flag_pixmap_(":/Resources/Flag.png"),
   pc_pixmap_(":/Resources/PC.png"),
   back_color_(220, 220, 220),
   address_color_(Qt::blue),
   mnemonic_color_(Qt::darkBlue),
   arg_color_(Qt::darkMagenta),
   byte_color_(Qt::darkGray),
   char_color_(Qt::gray)

{
   setFocusPolicy(Qt::StrongFocus);

   connect(&vertical_sb_, SIGNAL(valueChanged(int)), this, SLOT(OnValueChange(int)));

   margin_size_ = std::max(std::max(pc_pixmap_.width(), flag_pixmap_.width()), bp_pixmap_.width());
}

void DisassemblyWidget::SetDisassemblyInfo(Emulation* machine, unsigned short max_address)
{
   disassembler_ = machine->GetDisassembler();
   machine_ = machine->GetEngine();
   max_address_ = max_address;
}

void DisassemblyWidget::ForceTopAddress(unsigned short address)
{
   current_address_ = address;
   // Set scroller
   vertical_sb_.setValue(current_address_);

   // Update
   repaint();
}

void DisassemblyWidget::wheelEvent(QWheelEvent* event)
{
   QPoint numPixels = event->pixelDelta();
   QPoint numDegrees = event->angleDelta()/8;

   if (!numPixels.isNull()) 
   {
      // todo
    
   }
   else if (!numDegrees.isNull()) 
   {
      if (numDegrees.y() > 0)
      {
         for (int i = 0; i < numDegrees.y() / 15;  i++) GoUp();
      }
      else
      {
         if ((-1*numDegrees.y() / 15) < nb_lines_)
         {
            current_address_ = line_address_[-1*numDegrees.y() / 15];
         }
         else
         {
            current_address_ = line_address_.back();
         }
      }
   }
   repaint();
   event->accept();
}

void DisassemblyWidget::keyPressEvent(QKeyEvent* event)
{
   switch (event->key())
   {
   case Qt::Key_Down:
      current_address_ = line_address_[1];
      break;
   case Qt::Key_Up:
      GoUp();
      break;
   case Qt::Key_PageDown:
      current_address_ = line_address_.back();
      break;
   case Qt::Key_PageUp:
      for (int i = 0; i < nb_lines_;i++) GoUp();
      break;

   case Qt::Key_F2:
      // todo
      break;
   case Qt::Key_F9:
      // todo
      break;
   default:
      // todo
      break;
   }
   repaint();
}

void DisassemblyWidget::mousePressEvent(QMouseEvent* event)
{
   // Check line
   unsigned int line = event->y() / line_height_;

   // check position
   unsigned int x = event->x();

   // todo : set selection / breakpoints

}

unsigned short DisassemblyWidget::GetMaxedPreviousValidAdress(unsigned short Addr_P)
{
   Z80* z80 = machine_->GetProc();
   unsigned short address = Addr_P;
   unsigned short best_address = Addr_P;
   for (int i = 1; i <= 8; i++)
   {
      address = Addr_P - i;
      if (z80->GetOpcodeSize(address) == i)
      {
         best_address = address;
      }
   }
   return best_address;
}

unsigned short DisassemblyWidget::GetPreviousValidAdress(unsigned short Addr_P)
{
   Z80* z80 = machine_->GetProc();
   unsigned short address = Addr_P;
   unsigned short adress_degrade = 0;
   unsigned short best_address = Addr_P;
   unsigned int offset = 0;
   bool bIsValid = false;
   while ((!bIsValid) && address > 0 && offset < 8)
   {
      --address;
      ++offset;
      // Test if this adress size is ok
      if (z80->GetOpcodeSize(address) == offset)
      {
         // it fits !
         best_address = address;
      }
      else if (z80->GetOpcodeSize(address) < offset)
      {
         if (best_address == Addr_P)
         {
            best_address = address;
         }
      }
   }
   return best_address;
}

void DisassemblyWidget::GoUp()
{
   Z80* z80 = machine_->GetProc();
   unsigned short addr_Tmp = GetMaxedPreviousValidAdress(current_address_);
   if (addr_Tmp == current_address_)
   {
      current_address_ = GetPreviousValidAdress(current_address_);
   }
   else
   {
      current_address_ = addr_Tmp;
   }
}

void DisassemblyWidget::paintEvent(QPaintEvent* /* event */)
{
   QPainter painter(this);
   const int width = size().width() - 3;
   const int height = size().height() - 5;

   painter.fillRect(0, 0, width, height, back_color_);

   // Draw every lines 
   unsigned short line_address = current_address_;
   line_address_.clear();
   char mnemonic[16];
   char address[16];
   char arg[16];     

   sprintf(address, "0x%4.4X: ", 0xFFFF);
   const QFontMetrics fm(property("font").value<QFont>());
   const unsigned int address_size = fm.horizontalAdvance(address);
   const unsigned int char_size = fm.horizontalAdvance(' ');

   
   const unsigned int top_margin = 10;

   // Generic display : A line is composed of :
   // A flag/breakpoint
   // Current address
   // Mnemonic / argument of the code
   // Bytes of the current command (5 max)
   // Ascii char, if relevant (otherwise, a basic '.' )
   for (unsigned int i = 0; i < nb_lines_; i++)
   {
      // Display flag ?
      if (false)
      {
         // todo
         painter.drawPixmap(0, top_margin + line_height_ * i, flag_pixmap_);
      }
      
      // Display breakpoint ?
      if (false)
      {
         // todo
         painter.drawPixmap(0, top_margin + line_height_ * i, bp_pixmap_);
      }

      // Display execution arrow
      if (machine_->GetProc()->pc_ == line_address)
      {
         painter.drawPixmap(0, top_margin + line_height_ * i, pc_pixmap_);
      }

      // Address 
      sprintf(address, "%4.4X: ", line_address);
      painter.setPen(address_color_);
      painter.drawText(margin_size_, top_margin + line_height_ * i, address);

      // Mnemonic
      const int size = disassembler_->DasmMnemonic(line_address, mnemonic, arg);
      painter.setPen(mnemonic_color_);
      painter.drawText(margin_size_ + address_size, top_margin + line_height_ * i, mnemonic);
      // Arguments
      const unsigned int mnemonic_size = fm.horizontalAdvance(mnemonic);
      painter.setPen(arg_color_);
      painter.drawText(margin_size_ + address_size + mnemonic_size + char_size, top_margin + line_height_ * i, arg);

      // Bytes 
      char byte_buffer[16] = { 0 };
      char char_buffer[6] = { 0 };
      for (int j = 0; j < size && j < 5; j++)
      {
         char byte[4] = { 0 };
         unsigned char b = machine_->GetMem()->Get(line_address + j);
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
      painter.setPen(byte_color_);
      painter.drawText(margin_size_ + char_size * 30, top_margin + line_height_ * i, byte_buffer);
      // Character (if displayable)
      painter.setPen(char_color_);
      painter.drawText(margin_size_ + char_size * 45, top_margin  + line_height_ * i, char_buffer);

      // Current selected line
      // todo

      line_address_.push_back(line_address);

      line_address += size;
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
   const QFontMetrics fm (property("font").value<QFont>());

   line_height_ = fm.lineSpacing();
   nb_lines_ = (size().height() - horizontal_sb_.sizeHint().height()) / line_height_;
   line_address_.resize(nb_lines_);

   // Vertical:
   // Compute number of lines : 0 -> max_address_
   vertical_sb_.setMaximum(static_cast<int>((max_address_ > (nb_lines_ - 1))?(max_address_ - (nb_lines_-1)):(0)));
   vertical_sb_.setMinimum(0);
   vertical_sb_.setSingleStep(1);
   vertical_sb_.setPageStep(static_cast<int>(nb_lines_));
   vertical_sb_.setValue(static_cast<int>(current_address_));

}

