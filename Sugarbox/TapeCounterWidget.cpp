#include "TapeCounterWidget.h"

#include <QLineEdit>
#include <QPainter>
#include <QResizeEvent>

#include "SugarboxApp.h"
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
   size_.setWidth(fm.horizontalAdvance(counter_text_.c_str()) + 16);
   size_.setHeight(fm.lineSpacing());

   cb_.setEditable(true);
   cb_.installEventFilter(this);
   cb_.setValidator(new QDateValidator());
   cb_.hide();

   connect(&cb_, &QComboBox::currentIndexChanged, this, [=]() {SelectionChanged();});

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
   if (event->type() == QEvent::FocusOut && object == &cb_)
   {
      QWidget* FocusWidget = qApp->focusWidget();
      if ( (!FocusWidget) || (!FocusWidget->parentWidget()) || FocusWidget->parentWidget()->parentWidget() != &cb_)
      {
         // close edit
         EndCounterEdit();
      }
   }
   else if (event->type() == QEvent::KeyPress)
   {
      if ((QComboBox*)object == &cb_)
      {
         if (((QKeyEvent*)event)->key() == Qt::Key_Escape)
         {
            CancelCounterEdit();
            return true;
         }
         else if (((QKeyEvent*)event)->key() == Qt::Key_Return || ((QKeyEvent*)event)->key() == Qt::Key_Enter)
         {
            EndCounterEdit();
            return true;
         }
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

void TapeCounterWidget::SelectionChanged()
{
   if (tape_)
   {
      tape_->SetTapePosition(value_indexed_[cb_.currentIndex()]);
   }
   Update();
}

void TapeCounterWidget::CancelCounterEdit()
{
   cb_.hide();
   Update();
}

void TapeCounterWidget::EndCounterEdit()
{
   // Get cb value, and convert it
   QString str = cb_.currentText();
   auto byte_array = str.toLocal8Bit(); // here you create a bytearray
   const char* ptr = byte_array.constData();
   const int val = TextToPos(ptr);

   tape_->SetTapePosition(val);
   cb_.hide();
   Update();
}

void TapeCounterWidget::mousePressEvent(QMouseEvent* event)
{
   // Open a menu, with the part of the tape selection
   if (tape_)
   {
      // Create editable combobox
      cb_.clear();
      cb_.move(x(), y());
      cb_.resize(width(), height());

      
      // Add last eject position
      int last_pos = tape_->GetLastEjectPosition();

      std::string buffer;
      buffer.resize(16);

      value_indexed_.resize(tape_->GetNbBlocks());
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
         value_indexed_[i] = tape_->GetBlockPosition(i);
         cb_.addItem(buffer.c_str());
      }
      // current text : current index
      const unsigned int counter = tape_->GetCounter();
      char buf[16];
      sprintf(buf, ("%2.2i:%2.2i"), (counter) / 60, counter % 60);
      cb_.setCurrentText(counter_text_.c_str());

      cb_.show();
      cb_.setFocus();

   }
}