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
   crtc_list_(nullptr)
{
   parent_ = parent;

   informations_group_ = new QGroupBox(tr("Generic informations"));

   register_group_ = new QGroupBox(tr("Registers"));
   layout_reg_ = new QGridLayout;
   int current_row = 0;
   int current_column = 0;
   for (unsigned int i = 0; i < 18; i++)
   {
      register_edit_ [i] = new QLineEdit(QString(""));
      register_edit_[i]->setInputMask("HH");
      connect(register_edit_[i], &QLineEdit::editingFinished, this, [=]() {UpdateRegister(register_edit_[i]->text(), i); });
      register_label_[i] = new QLabel(QString("R%1").arg(i, 2, 10, QChar('0')), this);
      layout_reg_->addWidget(register_label_[i], current_row, current_column++);
      layout_reg_->addWidget(register_edit_[i], current_row, current_column++);
      if (current_column > 16)
      {
         current_column = 0;
         current_row++;
      }
   }
   register_group_->setLayout(layout_reg_);

   counters_group_ = new QGroupBox(tr("Counters"));
   layout_counters_ = new QGridLayout;

   layout_counters_->addWidget(new QLabel(QString("Vcc")), 0, 0);
   layout_counters_->addWidget(new QLineEdit(QString("")), 0, 1);
   
   counters_group_->setLayout(layout_counters_);

   layout_ = new QGridLayout;
   layout_->addWidget(informations_group_, 0, 0);
   layout_->addWidget(register_group_, 1, 0);
   layout_->addWidget(counters_group_, 2, 0);
   setLayout(layout_);

}

CRTCDialog::~CRTCDialog()
{
   // delete everything ?
   // todo
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

void CRTCDialog::SetEmulator(Emulation* emu_handler, MultiLanguage* language)
{
   language_ = language;
   emu_handler_ = emu_handler;
   emu_handler_->AddUpdateListener(this);

   // Update group labels
   informations_group_->setTitle(language_->GetString("L_DEBUG_CRTC_GEN_INFO"));
   register_group_->setTitle(language_->GetString("L_DEBUG_CRTC_REGISTERS"));
   counters_group_->setTitle(language_->GetString("L_DEBUG_CRTC_COUNTERS"));

   // Attach each register/counter to each value
   crtc_list_ = emu_handler->GetEngine()->GetCRTC()->registers_list_;

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
   // Update
   for (int i = 0; i < 18; i++)
   {
      register_edit_[i]->setText(QString("%1").arg(crtc_list_[i], 2, 10, QChar('0')));
   }



   // Update parent window
   parent_->repaint();
}


void CRTCDialog::SetAddress(unsigned int addr)
{
}

