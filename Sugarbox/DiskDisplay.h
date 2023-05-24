#pragma once

#include <QWidget>

#include "Emulation.h"
#include "Settings.h"

class DiskDisplay : public QWidget
{
   Q_OBJECT

public:
   explicit DiskDisplay(int index, QWidget* parent = nullptr);

   void SetEmulation(Emulation* emulation, Settings* settings);
   void Update();

   QSize	sizeHint() const override;

protected:
   void paintEvent(QPaintEvent* event) override;
   void resizeEvent(QResizeEvent* e) override;

private slots:

private:
   Emulation* emulation_;
   int index_;
   int status_;
   int current_track_;
   int current_sector_;
   // Cache for display
   QColor c1_, c2_;
   QLinearGradient linearGrad_;
   QPainterPath path1_;
   Settings* settings_;

   QString drive_;
   QString track_;
   QString sector_;
   std::string current_dump_;
};
