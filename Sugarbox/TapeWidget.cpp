#include "TapeWidget.h"

#include <QPainter>
#include <QResizeEvent>

TapeWidget::TapeWidget(ITapeInsertionCallback* callback, QWidget* parent) :
   QWidget(parent),
   callback_(callback),
   emulation_(nullptr),
   record_(QIcon(":/Resources/png7.png"), "", this),
   play_(QIcon(":/Resources/png8.png"), "", this),
   rewind_(QIcon(":/Resources/png9.png"), "", this),
   fast_forward_(QIcon(":/Resources/png10.png"), "", this),
   stop_(QIcon(":/Resources/png11.png"), "", this),
   pause_(QIcon(":/Resources/png12.png"), "", this),
   insert_(QIcon(":/Resources/png13.png"), "", this)
   
{
   status_layout_ = new QGridLayout(this);
   status_layout_->addWidget(&record_, 0, 1);
   status_layout_->addWidget(&play_, 0, 2);
   status_layout_->addWidget(&rewind_, 0, 3);
   status_layout_->addWidget(&fast_forward_, 0, 4);
   status_layout_->addWidget(&stop_, 0, 5);
   status_layout_->addWidget(&pause_, 0, 6);
   status_layout_->addWidget(&insert_, 0, 7);

}

void TapeWidget::SetEmulation(Emulation* emulation)
{
   emulation_ = emulation;
   connect(&record_, &QPushButton::released, this, [=]() {if (emulation_) emulation_->TapeRecord(); });
   connect(&play_, &QPushButton::released, this, [=]() {if (emulation_) emulation_->TapePlay(); });
   connect(&rewind_, &QPushButton::released, this, [=]() {if (emulation_) emulation_->TapeRewind(); });
   connect(&fast_forward_, &QPushButton::released, this, [=]() {if (emulation_) emulation_->TapeFastForward(); });
   connect(&stop_, &QPushButton::released, this, [=]() {if (emulation_) emulation_->TapeStop(); });
   connect(&pause_, &QPushButton::released, this, [=]() {if (emulation_) emulation_->TapePause(); });
   connect(&insert_, &QPushButton::released, this, [=]() {callback_->TapeInsert(); });


}

QSize	TapeWidget::sizeHint() const
{
   return status_layout_->sizeHint();
}


void TapeWidget::paintEvent(QPaintEvent* event)
{
   QPainter painter(this);
}

void TapeWidget::resizeEvent(QResizeEvent* e)
{
   QWidget::resizeEvent(e);
}
