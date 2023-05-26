#pragma once

#include <QWidget>
#include <QScrollBar>

#include "MemoryViewWidget.h"

class StackViewWidget : public MemoryViewWidget
{
   Q_OBJECT

public:
   explicit StackViewWidget(QWidget* parent = nullptr);


protected:
   void paintEvent(QPaintEvent* event) override;
   void resizeEvent(QResizeEvent* e) override;

public slots:


private:
   QPixmap pc_pixmap_;

};
