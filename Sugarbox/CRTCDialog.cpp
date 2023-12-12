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
   crtc_list_(nullptr), 
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
      disconnect(it.edit_);
      delete it.edit_;
      delete it.label_;
   }

   edit_list_.clear();

   delete informations_group_; informations_group_ = nullptr;
   delete register_group_; register_group_ = nullptr;
   delete counters_group_; counters_group_ = nullptr;

   delete layout_reg_; layout_reg_ = nullptr;
}

void CRTCDialog::UpdateRegister(QString str, unsigned int i)
{
   // Update only on break
   if (crtc_list_)
   {
      bool ok;
      unsigned int value = str.toUInt(&ok, 16);
      if (ok)
      {
         crtc_list_[i] = value;
      }
   }
}

void CRTCDialog::CreateEditValue(QGridLayout *layout, QString l, int row, int column, QString input_mask, unsigned char* ptr_current, unsigned char* ptr_old )
{
   EditValue edit_value;
   edit_value.label_ = new QLabel(l, this);
   layout->addWidget(edit_value.label_, row, column);
   edit_value.edit_ = new QLineEdit(QString(""), this);
   edit_value.edit_->setInputMask(input_mask);
   layout->addWidget(edit_value.edit_, row, column + 1);
   edit_value.value = ptr_current;
   edit_value.old_value = ptr_old;
   edit_list_.push_back(edit_value );

   connect(edit_value.edit_, &QLineEdit::editingFinished, this, [=]() mutable {bool ok; unsigned int value = edit_value.edit_->text().toUInt(&ok, 16);
                                                                                                      if (ok) { *edit_value.value = static_cast<unsigned char>(value & 0xFF); }});
}

void CRTCDialog::SetEmulator(Emulation* emu_handler, MultiLanguage* language)
{
   language_ = language;
   emu_handler_ = emu_handler;
   emu_handler_->AddUpdateListener(this);
   crtc_list_ = emu_handler->GetEngine()->GetCRTC()->registers_list_;

   // Clean up all elements
   Cleanup();

   informations_group_ = new QGroupBox(tr("Generic informations"));

   register_group_ = new QGroupBox(tr("Registers"));
   layout_reg_ = new QGridLayout;
   int current_row = 0;
   int current_column = 0;
   for (unsigned int i = 0; i < 18; i++)
   {
      CreateEditValue(layout_reg_, QString("R%1").arg(i, 2, 10, QChar('0')), current_row, current_column, "HH", &crtc_list_[i], &crtc_list_old_[i]);
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

   CreateEditValue(layout_counters_, QString("Vcc"), 0, 0, "HH", &emu_handler->GetEngine()->GetCRTC()->vcc_, &old_vcc_);

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

void CRTCDialog::UpdateValue(QLineEdit* edit, unsigned char new_value, bool is_new, bool is_running)
{
   QPalette palette;
   palette.setColor(QPalette::Base, settings_->GetColor(SettingsValues::BACK_COLOR));
   palette.setColor(QPalette::Text, is_new ? settings_->GetColor(SettingsValues::EDIT_TEXT_CHANGED) : settings_->GetColor(SettingsValues::EDIT_TEXT));
   edit->setPalette(palette);
   edit->setText(QString("%1").arg(new_value, 2, 10, QChar('0')));
   edit->setReadOnly(is_running);
}

void CRTCDialog::UpdateDebug()
{
   if (emu_handler_ != nullptr)
   {
      for (auto it : edit_list_)
      {
         UpdateValue(it.edit_, *it.value, *it.value != *it.old_value, emu_handler_->IsRunning());
         *it.old_value = *it.value;
      }
   }

   // Update parent window
   parent_->repaint();
}


void CRTCDialog::SetAddress(unsigned int addr)
{
}

