#include "TapeWidget.h"

#include <QPainter>
#include <QResizeEvent>

TapeWidget::TapeWidget(ITapeInsertionCallback* callback, QWidget* parent) :
   QWidget(parent),
   callback_(callback),
   emulation_(nullptr),
   record_( this),
   play_(this),
   rewind_(this),
   fast_forward_(this),
   stop_(this),
   pause_(this),
   insert_(this)
   
{

   record_.setIcon(QIcon(":/Resources/png7.png"));
   play_.setIcon(QIcon(":/Resources/png8.png"));
   rewind_.setIcon(QIcon(":/Resources/png9.png"));
   fast_forward_.setIcon(QIcon(":/Resources/png10.png"));
   stop_.setIcon(QIcon(":/Resources/png11.png"));
   pause_.setIcon(QIcon(":/Resources/png12.png"));
   insert_.setIcon(QIcon(":/Resources/png13.png"));

   status_layout_ = new QHBoxLayout(this);
   status_layout_->setSpacing(0);
   status_layout_->setContentsMargins(0, 0, 0, 0);

   // Add tape counter
   status_layout_->addWidget(&tape_counter_);
   // Add tape buttons
   status_layout_->addWidget(&record_);
   status_layout_->addWidget(&play_);
   status_layout_->addWidget(&rewind_);
   status_layout_->addWidget(&fast_forward_);
   status_layout_->addWidget(&stop_);
   status_layout_->addWidget(&pause_);
   status_layout_->addWidget(&insert_);
   


}

void TapeWidget::SetEmulation(Emulation* emulation)
{
   emulation_ = emulation;
   tape_counter_.SetTape(emulation->GetEngine()->GetTape());

   connect(&record_, &QToolButton::released, this, [=]() {if (emulation_) emulation_->TapeRecord(); });
   connect(&play_, &QToolButton::released, this, [=]() {if (emulation_) emulation_->TapePlay(); });
   connect(&rewind_, &QToolButton::released, this, [=]() {if (emulation_) emulation_->TapeRewind(); });
   connect(&fast_forward_, &QToolButton::released, this, [=]() {if (emulation_) emulation_->TapeFastForward(); });
   connect(&stop_, &QToolButton::released, this, [=]() {if (emulation_) emulation_->TapeStop(); });
   connect(&pause_, &QToolButton::released, this, [=]() {if (emulation_) emulation_->TapePause(); });
   connect(&insert_, &QToolButton::released, this, [=]() {callback_->TapeInsert(); });


}

QSize	TapeWidget::sizeHint() const
{
   return QWidget::sizeHint();
}


void TapeWidget::paintEvent(QPaintEvent* event)
{
   QPainter painter(this);
}

void TapeWidget::resizeEvent(QResizeEvent* e)
{
   QWidget::resizeEvent(e);
}
