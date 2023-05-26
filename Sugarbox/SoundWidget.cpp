#include "SoundWidget.h"

#include <QPainter>
#include <QResizeEvent>

SoundWidget::SoundWidget(ISoundNotification* sound_notification, MultiLanguage* language, QWidget* parent) :
   QWidget(parent), 
   sound_notification_(sound_notification), 
   language_(language),
   emulation_(nullptr),
   sound_changing_on_(false), 
   sound_on_("Resources/Media-Controls-High-volume-icon.png"),
   sound_muted_("Resources/Media-Controls-Mute-icon.png"),
   record_("Resources/Record-off-256.png"),
   record_pressed_("Resources/Record-Pressed-icon.png"),
   pos_mute_x_(0),
   pos_mute_y_(0),
   pos_record_x_(0),
   pos_record_y_(0)
{
}

void SoundWidget::SetEmulation(Emulation* emulation)
{
   emulation_ = emulation;
}

QSize	SoundWidget::sizeHint() const
{
   // Hint : width of pixamp of sound mute; 64 for sound bar; 4 for margins
   QSize s;
   s.setWidth(64 + sound_on_.width() + record_.width() + 10);
   s.setHeight(24);
   return s;

}

void SoundWidget::resizeEvent(QResizeEvent* e)
{
   // Compute path for sound bar drawing
   pos_mute_x_ = 70;
   pos_record_x_ = pos_mute_x_ + sound_on_.width() + 3;
   pos_mute_y_ = (e->size().height() - sound_on_.height()) / 2;
   pos_record_y_ = (e->size().height() - record_.height()) / 2;

   ComputePath();
   QWidget::resizeEvent(e);
}

void SoundWidget::ComputePath()
{

   float f = sound_notification_->GetVolume();
   int x1 = 1;
   int x2 = 1 + 64 * f;
   int x3 = 65;
   int y1 = height() - 2;
   int y2 = (height() - 2) * (1 - f);
   int y3 = 1;

   path1_.clear();
   path1_.moveTo(x1, y1);
   path1_.lineTo(x2, y1);
   path1_.lineTo(x2, y2);
   path1_.lineTo(x1, y1);

   // part of the sound off
   path2_.clear();
   path2_.moveTo(x2, y1);
   path2_.lineTo(x2, y2);
   path2_.lineTo(x3, y3);
   path2_.lineTo(x3, y1);
   path2_.lineTo(x2, y1);

   // Surrounding
   path3_.clear();
   path3_.moveTo(x1, y1);
   path3_.lineTo(x3, y1);
   path3_.lineTo(x3, y3);
   path3_.lineTo(x1, y1);
}

void SoundWidget::paintEvent(QPaintEvent* event)
{
   QPainter painter(this);

   // draw sound
   if (sound_notification_)
   {
      // Draw mute icon
      painter.drawPixmap(pos_mute_x_, pos_mute_y_, sound_notification_->IsMuted()?sound_muted_:sound_on_);

      // Record icon
      painter.drawPixmap(pos_record_x_, pos_record_y_, sound_notification_->IsRecordOn() ? record_pressed_:record_);

      // Draw sound
      QColor c1, c2, c3;
      if (sound_notification_->IsMuted())
      {
         c1 = QColor(0xC0, 0xC0, 0xC0);
         c2 = QColor(0xC0, 0xC0, 0xC0);
         c3 = QColor(0xC0, 0xC0, 0xC0);
      }
      else
      {
         c1 = QColor(0, 0xff, 0);
         c2 = QColor(0xFF, 0xEC, 0x1C);
         c3 = QColor(0xff, 0, 0);
      }

      QLinearGradient linearGrad(0, 0, width()-24, 0);
      linearGrad.setColorAt(0, c1);
      linearGrad.setColorAt(0.8, c2);
      linearGrad.setColorAt(1, c3);

      painter.setBrush(linearGrad);
      painter.drawPath(path1_);
      painter.setBrush(QColor (0xC0, 0xC0, 0xC0));
      painter.drawPath(path2_);
      painter.setPen(Qt::black);
      painter.setBrush(Qt::NoBrush);
      painter.drawPath(path3_);

      if (sound_changing_on_)
      {
         float f = sound_notification_->GetVolume();
         char text[5] = " 00%";
         if (f == 1.0)
         {
            text[0] = '1';
         }
         else
         {
            text[1] = '0' + (f * 10);
            text[2] = '0' + ((int)(f * 100) % 10);
         }
         painter.drawText(20,height()-4, text);
      }
   }
}


void SoundWidget::mousePressEvent(QMouseEvent* event)
{
   if (sound_notification_)
   {
      if (event->x() < 64)
      {
         float f = ((float)event->x()) / (64.0);
         sound_notification_->SetVolume(f);
         sound_changing_on_ = true;
         ComputePath();
         grabMouse();
      }
      else
      {
         if (event->x() < pos_record_x_)
         {
            sound_notification_->Mute(!sound_notification_->IsMuted());
         }
         else
         {
            sound_notification_->Record(!sound_notification_->IsRecordOn());
         }
         
      }
      repaint();
   }
   QWidget::mousePressEvent(event);
}

void SoundWidget::mouseReleaseEvent(QMouseEvent* event)
{
   releaseMouse();

   sound_changing_on_ = false;
   QWidget::mouseReleaseEvent(event);
}

void SoundWidget::mouseMoveEvent(QMouseEvent* event)
{
   QWidget::mouseMoveEvent(event);
   if (sound_changing_on_)
   {
      if (event->x() >= 1 && event->x() <= 65)
      {
         float f = ((float)event->x()-1) / (64.0);
         sound_notification_->SetVolume(f);
         ComputePath();
      }
      repaint();
   }
}
