#include <iomanip>

#include <QPainter>
#include <QDir>
#include <QMenuBar>
#include <QDebug>
#include <QMouseEvent>

#include "CRTCDialog.h"


CRTCDialog::CRTCDialog(QWidget *parent) :
   QDialog(nullptr),
   language_(nullptr),
   informations_group_(nullptr), 
   register_group_(nullptr),
   counters_group_(nullptr),
   layout_reg_(nullptr)
{
   parent_ = parent;


}

CRTCDialog::~CRTCDialog()
{
   Cleanup();
}

void CRTCDialog::Cleanup()
{
   for (auto& it : edit_list_)
   {
      disconnect(it->edit_);
      delete it->edit_;
      delete it->label_;
      delete it;
   }

   edit_list_.clear();

   delete informations_group_; informations_group_ = nullptr;
   delete register_group_; register_group_ = nullptr;
   delete counters_group_; counters_group_ = nullptr;

   delete layout_reg_; layout_reg_ = nullptr;
}

template<typename T>
void CRTCDialog::CreateEditValue(QWidget* parent, QGridLayout* layout, QString l, int row, int column, ValueEditCtrl::RegisterType reg_type, T* ptr_current)
{
   ValueEditCtrlImp<T> * edit_value = new ValueEditCtrlImp<T>(parent, settings_, reg_type);
   edit_value->label_ = new QLabel(l, this);
   layout->addWidget(edit_value->label_, row, column);
   layout->addWidget(edit_value->edit_, row, column + 1);
   edit_value->value_ = ptr_current;
   edit_list_.push_back(edit_value );

   connect(edit_value->edit_, &QLineEdit::editingFinished, this, [=]() mutable {edit_value->Validate(emu_handler_->IsRunning()); });
}

void CRTCDialog::SetEmulator(Emulation* emu_handler, MultiLanguage* language)
{
   language_ = language;
   emu_handler_ = emu_handler;
   emu_handler_->AddUpdateListener(this);

   // Clean up all elements
   Cleanup();

   informations_group_ = new QGroupBox(tr("Generic informations"));

   register_group_ = new QGroupBox(tr("Registers"));
   layout_reg_ = new QGridLayout;
   int current_row = 0;
   int current_column = 0;
   for (unsigned int i = 0; i < 18; i++)
   {
      CreateEditValue<unsigned char>(this, layout_reg_, QString("R%1").arg(i, 2, 10, QChar('0')), current_row, current_column, ValueEditCtrl::_8Bit, &emu_handler->GetEngine()->GetCRTC()->registers_list_[i]);
      current_column += 2;
      if (current_column > 16)
      {
         current_column = 0;
         current_row++;
      }
   }

   register_group_->setLayout(layout_reg_);

   counters_group_ = new QGroupBox(tr("Counters"));
   layout_counters_ = new QGridLayout;

   CreateEditValue<unsigned char>(this, layout_counters_, QString("Hcc"), 0, 0, ValueEditCtrl::_8Bit, &emu_handler->GetEngine()->GetCRTC()->hcc_);
   CreateEditValue<unsigned char>(this, layout_counters_, QString("Vlc"), 0, 2, ValueEditCtrl::_8Bit, &emu_handler->GetEngine()->GetCRTC()->vlc_);
   CreateEditValue<unsigned char>(this, layout_counters_, QString("Vcc"), 0, 4, ValueEditCtrl::_8Bit, &emu_handler->GetEngine()->GetCRTC()->vcc_);
   CreateEditValue<unsigned char>(this, layout_counters_, QString("R52"), 0, 6, ValueEditCtrl::_8Bit, &emu_handler->GetEngine()->GetVGA()->interrupt_counter_);
   CreateEditValue<unsigned char>(this, layout_counters_, QString("Vert. Adjust"), 0, 8, ValueEditCtrl::_8Bit, &emu_handler->GetEngine()->GetCRTC()->vertical_adjust_counter_);
   CreateEditValue<unsigned short>(this, layout_counters_, QString("MA"), 1, 0, ValueEditCtrl::_16Bit, &emu_handler->GetEngine()->GetCRTC()->ma_);
   CreateEditValue<unsigned char>(this, layout_counters_, QString("Addr. Reg."), 1, 2, ValueEditCtrl::_8Bit, &emu_handler->GetEngine()->GetCRTC()->adddress_register_);
   CreateEditValue<unsigned char>(this, layout_counters_, QString("Status Reg."), 1, 4, ValueEditCtrl::_8Bit, &emu_handler->GetEngine()->GetCRTC()->status_register_);
   CreateEditValue<unsigned char>(this, layout_counters_, QString("Horiz.Pulse"), 1, 6, ValueEditCtrl::_8Bit, &emu_handler->GetEngine()->GetCRTC()->horinzontal_pulse_);
   CreateEditValue<unsigned char>(this, layout_counters_, QString("Vert.Pulse"), 1, 8, ValueEditCtrl::_8Bit, &emu_handler->GetEngine()->GetCRTC()->scanline_vbl_);
   CreateEditValue<int>(this, layout_counters_, QString("Video Beam X"), 2, 0, ValueEditCtrl::_ValueInt, &emu_handler->GetEngine()->GetMonitor()->x_);
   CreateEditValue<int>(this, layout_counters_, QString("Video Beam Y"), 2, 2, ValueEditCtrl::_ValueInt, &emu_handler->GetEngine()->GetMonitor()->y_);

   counters_group_->setLayout(layout_counters_);

   layout_ = new QGridLayout;
   layout_->addWidget(informations_group_, 0, 0);
   layout_->addWidget(register_group_, 1, 0);
   layout_->addWidget(counters_group_, 2, 0);
   setLayout(layout_);

   // Update group labels
   informations_group_->setTitle(language_->GetString("L_DEBUG_CRTC_GEN_INFO"));
   register_group_->setTitle(language_->GetString("L_DEBUG_CRTC_REGISTERS"));
   counters_group_->setTitle(language_->GetString("L_DEBUG_CRTC_COUNTERS"));

   Update();
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
   if (emu_handler_ != nullptr)
   {
      for (auto it : edit_list_)
      {
         it->UpdateValue(emu_handler_->IsRunning());
      }
   }

   // Update parent window
   parent_->repaint();
}


void CRTCDialog::SetAddress(unsigned int addr)
{
}

