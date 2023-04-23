#include "DebugDialog.h"
#include "ui_DebugDialog.h"

#include <iomanip>

#include <QPainter>
#include <QDir>
#include <QMenuBar>
#include <QDebug>
#include <QMouseEvent>

DebugDialog::DebugDialog(QWidget *parent) :
   QDialog(parent),
   ui(new Ui::DebugDialog)
{
   ui->setupUi(this);
   ui->listWidget->setContextMenuPolicy(Qt::CustomContextMenu);
   ui->callStack->viewport()->installEventFilter(this);

   connect(ui->listWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(DasmShowContextMenu(QPoint)));


   QMetaObject::Connection result = connect(ui->callStack, SIGNAL(itemDoubleClicked(QListWidgetItem*, int)), SLOT(itemDoubleClicked(QListWidgetItem*)));

   ui->registers_list_->clear();
   ui->registers_list_->setColumnCount(2);
   ui->registers_list_->setRowCount(20);

   QTableWidgetItem  * item = new QTableWidgetItem("pc");
   ui->registers_list_->setItem(0, 0, item);

   item = new QTableWidgetItem("D0");
   ui->registers_list_->setItem(1, 0, item);
   ui->registers_list_->setItem(2, 0, new QTableWidgetItem("D1"));
   ui->registers_list_->setItem(3, 0, new QTableWidgetItem("D2"));
   ui->registers_list_->setItem(4, 0, new QTableWidgetItem("D3"));
   ui->registers_list_->setItem(5, 0, new QTableWidgetItem("D4"));
   ui->registers_list_->setItem(6, 0, new QTableWidgetItem("D5"));
   ui->registers_list_->setItem(7, 0, new QTableWidgetItem("D6"));
   ui->registers_list_->setItem(8, 0, new QTableWidgetItem("D7"));

   ui->registers_list_->setItem(9, 0, new QTableWidgetItem("A0"));
   ui->registers_list_->setItem(10, 0, new QTableWidgetItem("A1"));
   ui->registers_list_->setItem(11, 0, new QTableWidgetItem("A2"));
   ui->registers_list_->setItem(12, 0, new QTableWidgetItem("A3"));
   ui->registers_list_->setItem(13, 0, new QTableWidgetItem("A4"));
   ui->registers_list_->setItem(14, 0, new QTableWidgetItem("A5"));
   ui->registers_list_->setItem(15, 0, new QTableWidgetItem("A6"));
   ui->registers_list_->setItem(16, 0, new QTableWidgetItem("A7"));

   ui->registers_list_->setItem(17, 0, new QTableWidgetItem("SSP"));
   ui->registers_list_->setItem(18, 0, new QTableWidgetItem("USP"));

   ui->registers_list_->setItem(19, 0, new QTableWidgetItem("SR"));

   for (int i = 0; i < 20; i++)
   {
      ui->registers_list_->setItem(i, 1, new QTableWidgetItem("--"));
   }
   
}

DebugDialog::~DebugDialog()
{
}

bool DebugDialog::event(QEvent *event)
{
   if ( event->type() == QEvent::User )
   {
      UpdateDebug();
      return true;
   }
   return QWidget::event(event);
}

void DebugDialog::itemDoubleClicked(QListWidgetItem* item)
{
   int addr = item->data(Qt::UserRole).toInt();
   SetAddress(addr);
}

bool DebugDialog::eventFilter(QObject* watched, QEvent* event)
{
   if (watched == ui->callStack->viewport() && event->type() == QEvent::MouseButtonDblClick) {
      QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
      qDebug() << "MouseButtonDblClick" << mouseEvent->pos();
   }
   return QDialog::eventFilter(watched, event);
}

void DebugDialog::SetEmulator(Emulation* emu_handler)
{
   emu_handler_ = emu_handler;
   emu_handler_->AddUpdateListener(this);
   ui->listWidget->SetDisassemblyInfo(emu_handler, 0xFFFF);
}

void DebugDialog::Break()
{
   // wait for the emulator to be in a stable state
}

void DebugDialog::on_dbg_step__clicked()
{
   emu_handler_->Step();
}

void DebugDialog::on_dbg_run_clicked()
{
   emu_handler_->Run();
}

void DebugDialog::on_set_top_address_clicked()
{
   // get top 
   QString text = ui->dasm_address->text();
   if (text.length() > 0)
   {
      unsigned int addr = (unsigned int)strtoul(text.toUtf8().constData(), NULL, 16);
      if (addr >= 0 && addr < 0xFFFFFF)
      {
         // Set disassembly
         UpdateDisassembly(addr);

         // Write it on the log window
         std::string out_txt;
         //disassembler_.DisassembleArrayOfcode(emu_handler_->GetMotherboard(), addr, 512, out_txt);
         // Log it !
         qDebug() << out_txt.c_str();

      }
   }
}

void DebugDialog::on_dbg_pause_clicked()
{
   emu_handler_->Break();
}

void DebugDialog::Update()
{
   // Send "update" command
   QEvent* event = new QEvent(QEvent::User);
   QCoreApplication::postEvent( this, event);

}

void DebugDialog::StackToDasm()
{
   // double click on a stack value
   
   
}

void DebugDialog::DasmShowContextMenu(const QPoint &pos)
{
   // Handle global position
   QPoint globalPos = ui->listWidget->mapToGlobal(pos);

   // Create menu and insert some actions
   QMenu myMenu;
   myMenu.addAction("Insert breakpoint", this, SLOT(AddBreakpoint()));
   myMenu.addAction("Erase", this, SLOT(RemoveBreakpoint()));

   // Show context menu at handling position
   myMenu.exec(globalPos);
}

void DebugDialog::RemoveBreakpoint()
{
   // todo : handle multiple selection
   //emu_handler_->RemoveBreakpoint(list_of_items[0]->data(Qt::UserRole).toUInt());
   UpdateDebug();
}


void DebugDialog::AddBreakpoint()
{
   // todo : handle multiple selection
   //emu_handler_->AddBreakpoint(list_of_items[0]->data(Qt::UserRole).toUInt());
   UpdateDebug();
}

void DebugDialog::on_bpAddress_returnPressed()
{
   on_add_bp_clicked();
}

void DebugDialog::on_add_bp_clicked()
{
   QString text = ui->bpAddress->text();
   if (text.length() > 0)
   {
      //BreakPointHandler* pb_handler = emu_handler_->GetBreakpointHandler();
      //pb_handler->CreateBreakpoint(text.toUtf8().constData());
      UpdateDebug();
   }
}

void DebugDialog::on_remove_bp_clicked()
{
   QList<QListWidgetItem *> list_of_items = ui->list_breakpoints->selectedItems();
   if (list_of_items.size() > 0)
   {
      //BreakPointHandler* pb_handler = emu_handler_->GetBreakpointHandler();
      // todo : handle multiple selection
      //emu_handler_->RemoveBreakpoint(pb_handler->GetBreakpoint(list_of_items[0]->data(Qt::UserRole).toUInt()));
      UpdateDebug();
   }
}

void DebugDialog::on_clear_bp_clicked()
{
   //BreakPointHandler* pb_handler = emu_handler_->GetBreakpointHandler();
   //pb_handler->Clear();
   UpdateDebug();
}

void DebugDialog::UpdateDebug()
{
   unsigned int offset, offset_old;

   // Update parent window
   this->parentWidget()->repaint();

   // Get CPU
   Z80* z80 = emu_handler_->GetEngine()->GetProc();

   // Registers
   QString str = QString("%1").arg(z80->GetPC(), 6, 16);
   ui->registers_list_->item(0, 1)->setText( str );

   /*unsigned int stack_pointer = (m68k->GetDataSr() & 0x2000) ? z80->GetDataSsp() : m68k->GetDataUsp();
   for (int i = 0; i < 8; i++)
   {
      if (i != 7)
      {
         str = QString("%1").arg(m68k->GetAddressRegister(i), 6, 16);
      }
      else
      {
         
         // A7 is the stackpointer. Depends on either usp/ssp
         str = QString("%1").arg(stack_pointer, 6, 16);
      }

      ui->registers_list_->item(9+i, 1)->setText(str);

      str = QString("%1").arg(m68k->GetDataRegister(i), 6, 16);
      ui->registers_list_->item(1+i, 1)->setText(str);
   }

   str = QString("%1").arg(m68k->GetDataSsp(), 6, 16);
   ui->registers_list_->item(17, 1)->setText(str);
   str = QString("%1").arg(m68k->GetDataUsp(), 6, 16);
   ui->registers_list_->item(18, 1)->setText(str);
   str = QString("%1").arg(m68k->GetDataSr(), 6, 16);
   ui->registers_list_->item(19, 1)->setText(str);

   // CallStack
   unsigned char* mem = emu_handler_->GetMotherboard()->GetBus()->GetRam();
   str = QString("%1").arg(stack_pointer, 6, 16);
   offset = stack_pointer;
   ui->callStack->clear();
   if (stack_pointer < 512 * 1024)
   {
      for (int i = 0; i < 16; i++)
      {
         std::stringstream sstream;
         QListWidgetItem* stackitem = new QListWidgetItem;
         stackitem->setData(Qt::UserRole, offset);

         unsigned int dword_value = mem[offset+3] |
            (mem[offset + 2] << 8) |
            (mem[offset + 1] << 16) |
            (mem[offset] << 24);
                                    
         sstream << std::hex << std::uppercase << std::setfill('0') << std::setw(8) << offset;
         sstream << "  ";
         sstream << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << dword_value;

         stackitem->setText(sstream.str().c_str());
         stackitem->setData(Qt::UserRole, (int)dword_value);

         ui->callStack->addItem(stackitem);
         offset += 4;
      }
   }

   offset = offset_old = emu_handler_->GetMotherboard()->GetCpu()->GetPc() - 4; // -2 because of prefetch
   UpdateDisassembly(offset);

   // Breakpoints list
   BreakPointHandler* pb_handler = emu_handler_->GetBreakpointHandler();
   ui->list_breakpoints->clear();
   // Update breakpoints list
   for (int i = 0; i < pb_handler->GetBreakpointNumber(); i++)
   {
      QListWidgetItem* item = new QListWidgetItem;
      item->setText(pb_handler->GetBreakpoint(i)->GetLabel());
      item->setData(Qt::UserRole, i);
      
      ui->list_breakpoints->addItem(item);
   }
   */
}

// Update the disassembly windows : From offset, until the number of lines are completed
void DebugDialog::UpdateDisassembly(unsigned int offset)
{


}

void DebugDialog::SetAddress(unsigned int addr)
{
   UpdateDisassembly(addr);
}

