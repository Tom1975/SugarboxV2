#include <iomanip>

#include <QPainter>
#include <QDir>
#include <QMenuBar>
#include <QDebug>
#include <QMouseEvent>

#include "CRTCDialog.h"
#include "ui_CRTCDialog.h"


CRTCDialog::CRTCDialog(QWidget *parent) :
   QDialog(nullptr),
   ui(new Ui::CRTCDialog),
   language_(nullptr)
{
   parent_ = parent;
   ui->setupUi(this);

   registerGroup_ = new QGroupBox(tr("Registers"));

   // see https://doc.qt.io/qt-6/qtwidgets-widgets-lineedits-example.html
   QGridLayout* layout = new QGridLayout;
   layout->addWidget(registerGroup_, 0, 0);
   setLayout(layout);

}

CRTCDialog::~CRTCDialog()
{
}


void CRTCDialog::SetEmulator(Emulation* emu_handler, MultiLanguage* language)
{
   language_ = language;
   emu_handler_ = emu_handler;
   emu_handler_->AddUpdateListener(this);

   // Attach each register/counter to each value
   auto crtc_list = emu_handler->GetEngine()->GetCRTC()->registers_list_;

   // Add registers to CRTC dialog
   int current_row = 0;
   int current_column = 0;
   for (int i = 0; i < 18; i++)
   {
      ui->gridLayout->addWidget(new QLabel(QString("%1").arg(i, 2, 10, QChar('0')), this), current_row, current_column++);
      auto w = new QLineEdit(QString(""));
      ui->gridLayout->addWidget(w, current_row, current_column++);
      if (current_column > 16)
      {
         current_column = 0;
         current_row++;
      }
   }

   current_column = 0;
   current_row++;
   // Add internal counters
   ui->gridLayout->addWidget(new QLabel(QString("VCC"), this), current_row, current_column++);
   ui->gridLayout->addWidget(new QLineEdit(QString("")), current_row, current_column++);
   // Add videobeam position


}


bool CRTCDialog::event(QEvent *event)
{
   if ( event->type() == QEvent::User )
   {
      UpdateDebug();
      return true;
   }
   return QWidget::event(event);
}

void CRTCDialog::keyPressEvent(QKeyEvent* event)
{
   unsigned int k = ((event->key()) | event->modifiers());

   auto it = shortcuts_.find(k);
   if (it != shortcuts_.end())
   {
      it->second();
      repaint();
   }
   else
   {
      event->ignore();
   }
}

bool CRTCDialog::eventFilter(QObject* watched, QEvent* event)
{
   return QDialog::eventFilter(watched, event);
}

void CRTCDialog::SetSettings(Settings* settings)
{
   settings_ = settings;
}

void CRTCDialog::Update()
{
   // Send "update" command
   QEvent* event = new QEvent(QEvent::User);
   QCoreApplication::postEvent( this, event);
}

void CRTCDialog::showEvent(QShowEvent* event) 
{
   QWidget::showEvent(event);
   UpdateDebug();
}


void CRTCDialog::UpdateDebug()
{
   // Update parent window
   parent_->repaint();
}


void CRTCDialog::SetAddress(unsigned int addr)
{
}

