#include "DiskDisplay.h"

#include <QResizeEvent>

#include "DiskWidget.h"
#include "SettingsValues.h"

DiskDisplay::DiskDisplay(int index, QWidget* parent) :
   QWidget(parent),
   emulation_(nullptr),
   index_(index),
   status_(-1),
   current_track_(0),
   current_sector_(0),
   linearGrad_(1, 1, 20, 12),
   settings_(nullptr),
   drive_ (index_ == 0 ? "A" : "B"),
   track_("00"),
   sector_("00")
{
}

void DiskDisplay::SetEmulation(Emulation* emulation, Settings* settings)
{
   emulation_ = emulation;
   settings_ = settings;
   Update();
}

void DiskDisplay::Update()
{
   if (emulation_ != nullptr)
   {
      const int status = emulation_->GetEngine()->IsDiskRunning(index_);

      // 0 : Not present;  1 : No; 2 : Read; 3 : Write
      if (status != status_)
      {
         status_ = status;
         // Set status of protection button
         switch (status)
         {
         case 0:
            c1_ = settings_->GetColor(SettingsValues::DRIVE_STATUS_NODISK_1);
            c2_ = settings_->GetColor(SettingsValues::DRIVE_STATUS_NODISK_2);
            break;
         case 1:
            c1_ = settings_->GetColor(SettingsValues::DRIVE_STATUS_NO_ACTIVITY_1);
            c2_ = settings_->GetColor(SettingsValues::DRIVE_STATUS_NO_ACTIVITY_2);
            break;
         case 2:
            c1_ = settings_->GetColor(SettingsValues::DRIVE_STATUS_READ_1);
            c2_ = settings_->GetColor(SettingsValues::DRIVE_STATUS_READ_2);
            break;
         case 3:
            c1_ = settings_->GetColor(SettingsValues::DRIVE_STATUS_WRITE_1);
            c2_ = settings_->GetColor(SettingsValues::DRIVE_STATUS_WRITE_2);
            break;
         }
      }
      linearGrad_.setColorAt(0, c1_);
      linearGrad_.setColorAt(1, c2_);

      // Set sector/track
      const int current_track = emulation_->GetEngine()->GetFDC()->GetCurrentTrack(index_);
      if (current_track != current_track_)
      {
         current_track_ = current_track;
         track_ = QString("%1").arg(current_track_, 2, 10, QChar('0'));
      }
      const int current_sector = emulation_->GetEngine()->GetFDC()->GetCurrentSector(index_);
      if (current_sector_ != current_sector)
      {
         current_sector_ = current_sector;
         sector_ = QString("%1").arg(current_sector_, 2, 16, QChar('0'));
      }

      // Set tooltip with disk name
      std::string str = emulation_->GetEngine()->GetFDC()->GetDiskPath(index_);
      if (current_dump_ != str)
      {
         current_dump_ = str;
      }
   }
}

QSize	DiskDisplay::sizeHint() const
{

   return QSize(40, 24);
}


void DiskDisplay::paintEvent(QPaintEvent* event)
{
   QPainter painter(this);

   Update();
   painter.setBrush(linearGrad_);
   painter.setPen(Qt::black);
   painter.drawPath(path1_);

   painter.drawText(0, 0, 20, 12, Qt::AlignVCenter| Qt::AlignHCenter, drive_);
   painter.drawText(20, 24, track_);
   painter.drawText(20, 12,sector_);
}


void DiskDisplay::resizeEvent(QResizeEvent* e)
{
   path1_.clear();
   path1_.moveTo(1, e->size().height() - 2);
   path1_.lineTo(18, e->size().height() - 2);
   path1_.lineTo(18, 14);
   path1_.lineTo(1, 14);
   path1_.lineTo(1, e->size().height() - 2);

   QWidget::resizeEvent(e);
}
