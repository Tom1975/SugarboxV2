#pragma once

#include <QGridLayout>
#include <qtoolbutton.h>
#include <QWidget>
#include <QtWidgets/QLabel>

#include "Emulation.h"


class DiskWidget : public QWidget
{
   Q_OBJECT

public:
   explicit DiskWidget(QWidget* parent = nullptr);

   void SetEmulation(Emulation*);
   QSize	sizeHint() const override;

protected:
   void paintEvent(QPaintEvent* event) override;
   void resizeEvent(QResizeEvent* e) override;

private slots:

private:
   Emulation* emulation_;

   // Tape buttons
   QHBoxLayout*status_layout_;
};
