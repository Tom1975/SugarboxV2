#include <iomanip>

#include <QPainter>
#include <QDir>
#include <QMenuBar>
#include <QDebug>
#include <QMouseEvent>

#include "MemoryDialog.h"
#include "ui_MemoryDialog.h"


MemoryDialog::MemoryDialog(QWidget *parent) :
   QDialog(nullptr),
   ui(new Ui::MemoryDialog),
   language_(nullptr)
{
   parent_ = parent;
   ui->setupUi(this);

}

MemoryDialog::~MemoryDialog()
{
}

bool MemoryDialog::event(QEvent *event)
{
   if ( event->type() == QEvent::User )
   {
      UpdateDebug();
      return true;
   }
   return QWidget::event(event);
}

void MemoryDialog::keyPressEvent(QKeyEvent* event)
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

bool MemoryDialog::eventFilter(QObject* watched, QEvent* event)
{
   return QDialog::eventFilter(watched, event);
}

void MemoryDialog::SetEmulator(Emulation* emu_handler, MultiLanguage* language)
{
   language_ = language;
   emu_handler_ = emu_handler;
   emu_handler_->AddUpdateListener(this);
   ui->memoryWidget->SetDisassemblyInfo(emu_handler, 0xFFFF);
}

void MemoryDialog::SetFlagHandler(FlagHandler* flag_handler)
{
   flag_handler_ = flag_handler;
}

void MemoryDialog::SetSettings(Settings* settings)
{
   settings_ = settings;
}

void MemoryDialog::on_dasm_address_returnPressed()
{
   on_set_top_address_clicked();
}

void MemoryDialog::on_set_top_address_clicked()
{
   // get top 
   QString text = ui->dasm_address->text();
   if (text.length() > 0)
   {
      unsigned int addr = (unsigned int)strtoul(text.toUtf8().constData(), NULL, 16);
      if (addr >= 0 && addr < 0xFFFFFF)
      {
         ui->memoryWidget->ForceTopAddress(addr);
         // Write it on the log window
         std::string out_txt;
         qDebug() << out_txt.c_str();

      }
   }
}

void MemoryDialog::Update()
{
   // Send "update" command
   QEvent* event = new QEvent(QEvent::User);
   QCoreApplication::postEvent( this, event);

}

void MemoryDialog::showEvent(QShowEvent* event) 
{
   QWidget::showEvent(event);
   UpdateDebug();
}

void MemoryDialog::UpdateDebug()
{
   // Update parent window
   parent_->repaint();
}


void MemoryDialog::SetAddress(unsigned int addr)
{
}

