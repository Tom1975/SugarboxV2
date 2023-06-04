#include "TapeCounterWidget.h"

#include <QPainter>
#include <QResizeEvent>

#include "TapeWidget.h"


class QDateValidator : public QValidator
{
public:
   QDateValidator() : QValidator() {};

   virtual QValidator::State validate(QString& input, int& pos) const
   {
      int m, s;
      const auto ba = input.toLocal8Bit(); // here you create a bytearray
      const char* strin = ba.constData();

      if (sscanf(strin, "%i:%i", &m, &s) == 2)
      {
         if (s < 60 && s >= 0 && m >= 0 && m < 120)
            return QValidator::Acceptable;
         else
            return QValidator::Invalid;
      }
      else if (sscanf(strin, "%i:", &m) == 2)
      { 
         return QValidator::Intermediate;
      }
      // Only seconds
      else if (sscanf(strin, "%i", &s) == 1)
      {
         return QValidator::Acceptable;
      }
     
      return QValidator::Invalid;
   }


};

TapeCounterWidget::TapeCounterWidget(QWidget* parent) :
   QWidget(parent),
   tape_(nullptr),
   counter_text_("00:00/00:00"),
   cb_(this)
{
   counter_text_.reserve(12);
   // check size value
   const QFontMetrics fm(property("font").value<QFont>());
   size_.setWidth(fm.horizontalAdvance(counter_text_.c_str()));
   size_.setHeight(fm.lineSpacing());

   cb_.setEditable(true);
   cb_.installEventFilter(this);
   cb_.setValidator(new QDateValidator());
   cb_.hide();
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
         sprintf(&counter_text_[0], ("%2.2i:%2.2i/%2.2i:%2.2i"), (sec) / 60, sec % 60, max / 60, max % 60);
         repaint();
      }
   }
}

bool TapeCounterWidget::eventFilter(QObject* object, QEvent* event)
{
   if (event->type() == QEvent::FocusOut)
   {
      if (object == &cb_ )
      {
         // close edit
         EndCounterEdit();
      }
   }
   return false;
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

int TextToPos(const char* pBuffer)
{
   // Try to format....
   int m, s;

   // minutes / seconds
   if (sscanf(pBuffer, "%i:%i", &m, &s) == 2)
   {
      return m * 60 + s;
   }
   // Only seconds
   else if (sscanf(pBuffer, "%i", &s) == 1)
   {
      return s;
   }
   return 0;
}

void TapeCounterWidget::EndCounterEdit()
{
   // Get cb value, and convert it
   const auto ba = cb_.itemData(cb_.currentIndex()).toString().toLocal8Bit(); // here you create a bytearray
   int val = TextToPos(ba.constData());

   cb_.hide();
}

void TapeCounterWidget::mousePressEvent(QMouseEvent* event)
{
   // Open a menu, with the part of the tape selection
   if (tape_)
   {
      // Create editable combobox
      
      cb_.move(x(), y());
      cb_.resize(width(), height());

      
      // Add last eject position
      int last_pos = tape_->GetLastEjectPosition();

      std::string buffer;
      buffer.resize(16);
      for (int i = 0; i < tape_->GetNbBlocks(); i++)
      {
         const char* txt = tape_->GetTextBlock(i);
         if (txt == nullptr)
         {
            sprintf(&buffer[0], "Block %i", i);
         }
         else
         {
            buffer = txt;
         }
         cb_.addItem(buffer.c_str());
      }

      cb_.show();
      cb_.setFocus();

   }
}