#pragma once

#include <QGridLayout>
#include <qtoolbutton.h>
#include <QWidget>
#include <QtWidgets/QLabel>

#include "Emulation.h"

#include "ISoundNotification.h"
#include "MultiLanguage.h"

class SoundWidget : public QWidget
{
   Q_OBJECT

public:
   explicit SoundWidget(ISoundNotification* sound_notification, MultiLanguage* language, QWidget* parent = nullptr);

   void SetEmulation(Emulation*);
   QSize	sizeHint() const override;

protected:
   void paintEvent(QPaintEvent* event) override;
   void resizeEvent(QResizeEvent* e) override;
   void mousePressEvent(QMouseEvent* event) override;
   void mouseReleaseEvent(QMouseEvent* event) override;
   void mouseMoveEvent(QMouseEvent* event) override;


   void ComputePath();

private slots:

private:
   Emulation* emulation_;
   ISoundNotification* sound_notification_;
   MultiLanguage* language_;
   bool sound_changing_on_;

   QPixmap sound_on_;
   QPixmap sound_muted_;
   QPixmap record_;
   QPixmap record_pressed_;

   QPainterPath path1_;
   QPainterPath path2_;
   QPainterPath path3_;

   int pos_mute_x_;
   int pos_mute_y_;
   int pos_record_x_;
   int pos_record_y_;
};
