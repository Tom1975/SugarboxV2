#include "DiskWidget.h"

#include <QPainter>
#include <QResizeEvent>

DiskWidget::DiskWidget(QWidget* parent) :
   QWidget(parent),
   emulation_(nullptr),
   disk_a_(this),
   disk_b_(this),
   disk_a_protection_(this),
   disk_b_protection_(this)
{
   QIcon protected_icon;
   protected_icon.addPixmap(QPixmap(":/Resources/protec_off.png"), QIcon::Normal);
   protected_icon.addPixmap(QPixmap(":/Resources/protec_on.png"), QIcon::Normal);

   disk_a_.setIcon(QIcon(":/Resources/LoadDisk.bmp"));
   disk_b_.setIcon(QIcon(":/Resources/LoadDisk.bmp"));
   disk_a_protection_.setIcon(protected_icon);
   disk_a_protection_.setCheckable(true);
   disk_b_protection_.setIcon(protected_icon);
   disk_b_protection_.setCheckable(true);

   status_layout_ = new QHBoxLayout(this);
   status_layout_->setSpacing(0);
   status_layout_->setContentsMargins(0, 0, 0, 0);
   status_layout_->addWidget(&disk_a_);
   status_layout_->addWidget(&disk_a_protection_);
   status_layout_->addWidget(&disk_b_);
   status_layout_->addWidget(&disk_b_protection_);


}

void DiskWidget::SetEmulation(Emulation* emulation)
{
   emulation_ = emulation;

   connect(&disk_a_protection_, &QToolButton::released, this, [=]() {if (emulation_) { emulation_->GetEngine()->GetFDC()->SetWriteProtection(!emulation_->GetEngine()->GetFDC()->IsDiskWriteProtected(0), 0); }});
   connect(&disk_b_protection_, &QToolButton::released, this, [=]() {if (emulation_) { emulation_->GetEngine()->GetFDC()->SetWriteProtection(!emulation_->GetEngine()->GetFDC()->IsDiskWriteProtected(1), 1); }});
}

QSize	DiskWidget::sizeHint() const
{
   return QWidget::sizeHint();
}


void DiskWidget::paintEvent(QPaintEvent* event)
{
   QPainter painter(this);
}


void DiskWidget::resizeEvent(QResizeEvent* e)
{
   QWidget::resizeEvent(e);
}
