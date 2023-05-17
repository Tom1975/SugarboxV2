#pragma once

#include <QGridLayout>
#include <qtoolbutton.h>
#include <QWidget>
#include <QtWidgets/QLabel>

#include "Emulation.h"
#include "TapeCounterWidget.h"

class ITapeInsertionCallback
{
public:
   virtual void TapeInsert() = 0;
};

class TapeWidget : public QWidget
{
   Q_OBJECT

public:
   explicit TapeWidget(ITapeInsertionCallback* callback, QWidget* parent = nullptr);

   void SetEmulation(Emulation*);
   QSize	sizeHint() const override;

protected:
   void paintEvent(QPaintEvent* event) override;
   void resizeEvent(QResizeEvent* e) override;

private slots:

private:
   ITapeInsertionCallback* callback_;
   Emulation* emulation_;

   // Tape buttons
   QHBoxLayout*status_layout_;
   QIcon stop_icon_;

   TapeCounterWidget tape_counter_;
   QToolButton record_;
   QToolButton play_;
   QToolButton rewind_;
   QToolButton fast_forward_;
   QToolButton stop_;
   QToolButton pause_;
   QToolButton insert_;
};
