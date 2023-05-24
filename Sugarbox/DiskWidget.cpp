#include "DiskWidget.h"

#include <QPainter>
#include <QResizeEvent>

DiskWidget::DiskWidget(QWidget* parent) :
   QWidget(parent),
   emulation_(nullptr),
   disk_a_protection_(this),
   disk_b_protection_(this),
   disk_a_display_(0, this),
   disk_b_display_(1, this)
{
   QIcon protected_icon;
   protected_icon.addPixmap(QPixmap(":/Resources/protec_off.png"), QIcon::Normal);
   protected_icon.addPixmap(QPixmap(":/Resources/protec_on.png"), QIcon::Normal);

   disk_a_protection_.setIcon(protected_icon);
   disk_a_protection_.setCheckable(true);
   disk_b_protection_.setIcon(protected_icon);
   disk_b_protection_.setCheckable(true);

   status_layout_ = new QHBoxLayout(this);
   status_layout_->setSpacing(0);
   status_layout_->setContentsMargins(0, 0, 0, 0);

   status_layout_->addWidget(&disk_a_protection_);
   status_layout_->addWidget(&disk_a_display_);
   status_layout_->addWidget(&disk_b_protection_);
   status_layout_->addWidget(&disk_b_display_);
}

void DiskWidget::SetEmulation(Emulation* emulation, Settings* settings)
{
   emulation_ = emulation;

   connect(&disk_a_protection_, &QToolButton::released, this, [=]() {if (emulation_) { emulation_->GetEngine()->GetFDC()->SetWriteProtection(!emulation_->GetEngine()->GetFDC()->IsDiskWriteProtected(0), 0); }});
   connect(&disk_b_protection_, &QToolButton::released, this, [=]() {if (emulation_) { emulation_->GetEngine()->GetFDC()->SetWriteProtection(!emulation_->GetEngine()->GetFDC()->IsDiskWriteProtected(1), 1); }});

   disk_a_display_.SetEmulation(emulation_, settings);
   disk_b_display_.SetEmulation(emulation_, settings);

   Update();
}

void DiskWidget::Update()
{
   // Set status of protection button
   // Set tooltip with disk name

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
