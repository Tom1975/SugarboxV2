#include "TapeCounterWidget.h"

#include <QPainter>
#include <QResizeEvent>

#include "TapeWidget.h"

TapeCounterWidget::TapeCounterWidget(QWidget* parent) :
   QWidget(parent),
   tape_(nullptr),
   counter_text_("00:00/00:00")
{
   counter_text_.reserve(12);
   // check size value
   const QFontMetrics fm(property("font").value<QFont>());
   size_.setWidth(fm.horizontalAdvance(counter_text_.c_str()));
   size_.setHeight(fm.lineSpacing());

}

void TapeCounterWidget::SetTape(CTape* tape)
{
   tape_ = tape;
}

QSize	TapeCounterWidget::sizeHint() const
{
   return size_;
}

void TapeCounterWidget::Update()
{
   if (tape_)
   {
      int sec = tape_->GetCounter();
      int max = tape_->LengthOfTape();
      if ( sec_ != sec || max_ != max)
      {
         sec_ = sec;
         max_ = max;
         sprintf_s(&counter_text_[0], counter_text_.size() + 1, ("%2.2i:%2.2i/%2.2i:%2.2i"), (sec) / 60, sec % 60, max / 60, max % 60);
         repaint();
      }
   }
}

void TapeCounterWidget::paintEvent(QPaintEvent* event)
{
   QPainter painter(this);

   painter.setPen(Qt::black);
   painter.drawText(0, 0, size_.width(), size_.height(),Qt::AlignLeft | Qt::AlignVCenter, counter_text_.c_str());
}

void TapeCounterWidget::resizeEvent(QResizeEvent* e)
{
   size_ = e->size();
   QWidget::resizeEvent(e);
}

void TapeCounterWidget::mousePressEvent(QMouseEvent* event)
{
   // Open a menu, with the part of the tape selection
   if (tape_)
   {
      // Add last eject position
      int last_pos = tape_->GetLastEjectPosition();


      for (int i = 0; i < tape_->GetNbBlocks(); i++)
      {
         
      }
   }
}