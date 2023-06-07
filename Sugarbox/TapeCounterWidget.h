#pragma once

#include <QGridLayout>
#include <qtoolbutton.h>
#include <QWidget>
#include <QComboBox>

#include "Emulation.h"


class TapeCounterWidget : public QWidget
{
   Q_OBJECT

public:
   explicit TapeCounterWidget(QWidget* parent = nullptr);
   void SetTape(CTape* tape);
   void Update();

   QSize	sizeHint() const override;
   bool eventFilter(QObject* object, QEvent* event);

protected:
   void paintEvent(QPaintEvent* event) override;
   void resizeEvent(QResizeEvent* e) override;
   void mousePressEvent(QMouseEvent* event) override;

   void EndCounterEdit();
   void CancelCounterEdit();
   void SelectionChanged(int index);

private slots:

private:
   CTape* tape_;
   std::string counter_text_;
   int sec_;
   int max_;

   QSize size_;
   QComboBox cb_;

   std::vector<unsigned int> value_indexed_;
};
