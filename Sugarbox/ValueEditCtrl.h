#pragma once

#include <qlabel>
#include <qlineedit>

#include "Settings.h"
#include "SettingsValues.h"


class ValueEditCtrl
{
public:
   enum RegisterType
   {
      SingleBit,
      Bitfield,
      _8Bit,
      _16Bit,
      _16BitAdress,
      _ValueInt
   };

   virtual void UpdateValue(bool is_running) = 0;
   virtual void Validate(bool is_running) = 0;

   QLabel* label_;
   QLineEdit* edit_;
};


template <typename T>
class ValueEditCtrlImp : public ValueEditCtrl
{
public:
   
   ValueEditCtrlImp(QWidget* parent, Settings* settings, RegisterType reg_type) : parent_(parent), settings_(settings), reg_type_(reg_type), old_value_(0)
   {
      edit_ = new QLineEdit(QString(""), parent);
      switch (reg_type_)
      {
      case _8Bit:edit_->setInputMask("HH"); break;
      case _16Bit:edit_->setInputMask("HHHH"); break;
      case _ValueInt:edit_->setInputMask("0000"); break;

      }
   }

   virtual ~ValueEditCtrlImp()
   {
      delete edit_;
   }

   virtual void UpdateValue(bool is_running)
   {
      bool is_new = (*value_ != old_value_);

      QPalette palette;
      palette.setColor(QPalette::Base, settings_->GetColor(SettingsValues::BACK_COLOR));
      palette.setColor(QPalette::Text, is_running? SettingsValues::EDIT_TEXT_DISABLED : (is_new ? settings_->GetColor(SettingsValues::EDIT_TEXT_CHANGED) : settings_->GetColor(SettingsValues::EDIT_TEXT)));
      edit_->setPalette(palette);
      edit_->setText(QString("%1").arg(*value_, 2, 10, QChar('0')));
      edit_->setReadOnly(is_running);

      old_value_ = *value_;
   }

   virtual void Validate(bool is_running)
   {
      bool ok;

      T value = 0;
      QString txt = edit_->text();
      switch (reg_type_)
      {
      case _8Bit:
      case _16Bit:
         value = txt.toUInt(&ok, 16); 
         break;
      case _ValueInt:
         value = txt.toUInt(&ok, 10);
         break;
      }
      
      if (ok) {
         *value_ = value;
      }
   }

   T* value_;

private:
   T old_value_;
   RegisterType reg_type_;
   QWidget* parent_;
   Settings* settings_;
};

