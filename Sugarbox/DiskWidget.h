#pragma once

#include <QHBoxLayout>
#include <qtoolbutton.h>
#include <QWidget>

#include "Emulation.h"
#include "DiskDisplay.h"

class DiskWidget : public QWidget
{
   Q_OBJECT

public:
   explicit DiskWidget(QWidget* parent = nullptr);

   void SetEmulation(Emulation*, Settings* settings);
   void Update();

   QSize	sizeHint() const override;

protected:
   void paintEvent(QPaintEvent* event) override;
   void resizeEvent(QResizeEvent* e) override;

private slots:

private:
   Emulation* emulation_;
   QHBoxLayout*status_layout_;
   QToolButton disk_a_protection_;
   QToolButton disk_b_protection_;
   DiskDisplay disk_a_display_;
   DiskDisplay disk_b_display_;
};
