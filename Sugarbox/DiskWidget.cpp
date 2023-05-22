#include "DiskWidget.h"

#include <QPainter>
#include <QResizeEvent>

DiskWidget::DiskWidget(QWidget* parent) :
   QWidget(parent),
   emulation_(nullptr)
{
   status_layout_ = new QHBoxLayout(this);
   status_layout_->setSpacing(0);
   status_layout_->setContentsMargins(0, 0, 0, 0);
}

void DiskWidget::SetEmulation(Emulation* emulation)
{
   emulation_ = emulation;
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
