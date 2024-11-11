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
   QObject::connect(ui->bank, SIGNAL(currentIndexChanged(int)), SLOT(ChangeMemorySource(int)));

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

   UpdateMemoryCombo();
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

void MemoryDialog::UpdateMemoryCombo()
{

   ui->bank->addItem("READ Memory", ((Memory::MEM_READ) << 8) | 0 );
   ui->bank->addItem("WRITE Memory", ((Memory::MEM_WRITE) << 8) | 0);
   ui->bank->addItem("RAM Bank : Lower 64ko bank", ((Memory::MEM_RAM_LOWER_BANK) << 8) | 0);

   // High memory (if any)
   auto mem = emu_handler_->GetEngine()->GetMem();
   bool* available_ram = mem->GetAvailableRam();
   if (available_ram[0])
   {
      ui->bank->addItem("RAM Bank : Upper 64ko bank", ((Memory::MEM_RAM_BANK) << 8) | 0);
   }

   // Memory expansions
   for (int i = 1; i < 8; i++)
   {
      if (available_ram[i])
      {
         std::string label = "Expansion RAM Bank : " + std::to_string(i);
         ui->bank->addItem (label.c_str(), ((Memory::MEM_RAM_BANK) << 8) | i);
      }
   }

   // Specific ROM
   bool* rom_available = mem->GetAvailableROM();

   // Lower ROM ?
   if (mem->IsLowerRomLoaded())
   {
      ui->bank->addItem("Lower ROM", ((Memory::MEM_LOWER_ROM) << 8) | 0);
   }

   for (int i = 0; i < 32; i++)
   {
      if (rom_available[i])
      {
         std::string label = "ROM Bank : " + std::to_string(i);
         ui->bank->addItem(label.c_str(), ((Memory::MEM_ROM_BANK) << 8) | i);
      }
   }

   // Specific Cartridge
   bool* cart_available = mem->GetAvailableCartridgeSlot();

   for (int i = 0; i < 32; i++)
   {
      if (cart_available[i])
      {
         std::string label = "Cartridge Bank : " + std::to_string(i);
         ui->bank->addItem(label.c_str(), ((Memory::MEM_CART_SLOT) << 8) | i);
      }
   }
}

void MemoryDialog::ChangeMemorySource(int index)
{
   // Get proper ram source
   unsigned int data = ui->bank->currentData().toInt();
   ui->memoryWidget->SetMemoryToRead((Memory::DbgMemAccess)(data >> 8), data & 0xFF);
}

void MemoryDialog::UpdateDebug()
{
   // Update parent window
   parent_->repaint();
}


void MemoryDialog::SetAddress(unsigned int addr)
{
   ui->memoryWidget->ForceTopAddress(addr);
}

