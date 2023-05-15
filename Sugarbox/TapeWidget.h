#pragma once

#include <QGridLayout>
#include <QPushButton>
#include <QWidget>

#include "Emulation.h"

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
   QGridLayout *status_layout_;
   QIcon stop_icon_;
   QPushButton record_;
   QPushButton play_;
   QPushButton rewind_;
   QPushButton fast_forward_;
   QPushButton stop_;
   QPushButton pause_;
   QPushButton insert_;
};
