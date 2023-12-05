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
   QObject::connect(ui->bank, SIGNAL(currentIndexChanged(int)), SLOT(ChangeMemorySource(int)));

}

CRTCDialog::~CRTCDialog()
{
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

void CRTCDialog::SetEmulator(Emulation* emu_handler, MultiLanguage* language)
{
   language_ = language;
   emu_handler_ = emu_handler;
   emu_handler_->AddUpdateListener(this);
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
   ui->memoryWidget->ForceTopAddress(addr);
}

