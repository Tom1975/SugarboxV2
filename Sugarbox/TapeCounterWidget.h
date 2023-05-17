#pragma once

#include <QGridLayout>
#include <qtoolbutton.h>
#include <QWidget>

#include "Emulation.h"


class TapeCounterWidget : public QWidget
{
   Q_OBJECT

public:
   explicit TapeCounterWidget(QWidget* parent = nullptr);
   void SetTape(CTape* tape);

   QSize	sizeHint() const override;

protected:
   void paintEvent(QPaintEvent* event) override;
   void resizeEvent(QResizeEvent* e) override;
   void mousePressEvent(QMouseEvent* event) override;
   

private slots:

private:
   CTape* tape_;
   std::string counter_text_;

   QSize size_;
};
