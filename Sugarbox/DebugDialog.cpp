#include "DebugDialog.h"
#include "ui_DebugDialog.h"

#include <iomanip>

#include <QPainter>
#include <QDir>
#include <QMenuBar>
#include <QDebug>
#include <QMouseEvent>

#include <format>
template<typename T>
DebugDialog::RegisterView<T>::RegisterView(std::string label, T* reg) : label_(label), register_(reg), value_("----")
{

}

template<typename T>
std::string& DebugDialog::RegisterView<T>::GetLabel()
{
   return label_;
}

template<>
std::string& DebugDialog::RegisterView<unsigned short>::GetValue()
{
   std::snprintf(&value_[0], value_.size(), "%4.4X", register_);
   return value_;
}

template<>
std::string& DebugDialog::RegisterView<Z80::Register>::GetValue()
{
   std::snprintf(&value_[0], value_.size(), "%4.4X", register_->w);
   return value_;
}

DebugDialog::FlagView::FlagView(std::string label, Z80::Register* reg) : RegisterView(label, reg)
{
   value_ = "--------";
}


std::string& DebugDialog::FlagView::GetValue()
{
   // Draw the full flags
   return value_;
}


DebugDialog::DebugDialog(QWidget *parent) :
   QDialog(parent),
   ui(new Ui::DebugDialog)
{
   ui->setupUi(this);
   ui->listWidget->setContextMenuPolicy(Qt::CustomContextMenu);

   connect(ui->listWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(DasmShowContextMenu(QPoint)));

   // Set registers
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
   return QDialog::eventFilter(watched, event);
}

void DebugDialog::SetEmulator(Emulation* emu_handler)
{
   emu_handler_ = emu_handler;
   emu_handler_->AddUpdateListener(this);
   ui->listWidget->SetDisassemblyInfo(emu_handler, 0xFFFF);
   ui->memoryWidget->SetDisassemblyInfo(emu_handler, 0xFFFF);
   ui->stackWidget->SetDisassemblyInfo(emu_handler, 0xFFFF);

   // Set registers
   Z80* z80 = emu_handler->GetEngine()->GetProc();
   register_list_.push_back(new DebugDialog::FlagView("FLAG", &z80->af_));
   register_list_.push_back(new DebugDialog::RegisterView("AF", &z80->af_));
   register_list_.push_back(new DebugDialog::RegisterView("HL", &z80->hl_));
   register_list_.push_back(new DebugDialog::RegisterView("DE", &z80->de_));
   register_list_.push_back(new DebugDialog::RegisterView("BC", &z80->bc_));
   register_list_.push_back(new DebugDialog::RegisterView("AF'", &z80->af_p_));
   register_list_.push_back(new DebugDialog::RegisterView("HL'", &z80->hl_p_));
   register_list_.push_back(new DebugDialog::RegisterView("DE'", &z80->de_p_));
   register_list_.push_back(new DebugDialog::RegisterView("BC'", &z80->bc_p_));
   register_list_.push_back(new DebugDialog::RegisterView("IX", &z80->ix_));
   register_list_.push_back(new DebugDialog::RegisterView("IY", &z80->iy_));
   register_list_.push_back(new DebugDialog::RegisterView("SP", &z80->sp_));
   register_list_.push_back(new DebugDialog::RegisterView("PC", &z80->pc_));
   register_list_.push_back(new DebugDialog::RegisterView("I", &z80->ir_));

   ui->registers_list_->clear();
   ui->registers_list_->setColumnCount(2);
   ui->registers_list_->setRowCount(register_list_.size());

   for (unsigned int i = 0; i < register_list_.size(); i++)
   {
      ui->registers_list_->setItem(i, 0, new QTableWidgetItem(register_list_[i]->GetLabel().c_str()));
      ui->registers_list_->setItem(i, 1, new QTableWidgetItem(register_list_[i]->GetValue().c_str()));
   }

}

void DebugDialog::SetFlagHandler(FlagHandler* flag_handler)
{
   flag_handler_ = flag_handler;
   ui->listWidget->SetFlagHandler(flag_handler_);
}

void DebugDialog::Break()
{
   // wait for the emulator to be in a stable state

}
void DebugDialog::on_dasm_address_returnPressed()
{
   on_set_top_address_clicked();
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

void DebugDialog::UpdateDebug()
{
   unsigned int offset, offset_old;

   // Update parent window
   this->parentWidget()->repaint();

   // Get CPU
   Z80* z80 = emu_handler_->GetEngine()->GetProc();

   // Registers

   QString str = QString("%1").arg(z80->GetPC(), 6, 16);

   for (unsigned int i = 0; i < register_list_.size(); i++)
   {
      ui->registers_list_->item(i, 0)->setText(register_list_[i]->GetLabel().c_str());
      ui->registers_list_->item(i, 1)->setText(register_list_[i]->GetValue().c_str());
   }


}

// Update the disassembly windows : From offset, until the number of lines are completed
void DebugDialog::UpdateDisassembly(unsigned int offset)
{
   ui->listWidget->ForceTopAddress(offset);
}

void DebugDialog::SetAddress(unsigned int addr)
{
   UpdateDisassembly(addr);
}

