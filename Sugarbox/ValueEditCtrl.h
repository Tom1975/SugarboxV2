#pragma once

#include <qlabel>
#include <qlineedit>

#include "Settings.h"
#include "SettingsValues.h"

#define BYTETOBINARYPATTERN "%d%d%d%d%d%d%d%d"
#define BYTETOBINARY(byte)  \
  (byte & 0x80 ? 1 : 0), \
  (byte & 0x40 ? 1 : 0), \
  (byte & 0x20 ? 1 : 0), \
  (byte & 0x10 ? 1 : 0), \
  (byte & 0x08 ? 1 : 0), \
  (byte & 0x04 ? 1 : 0), \
  (byte & 0x02 ? 1 : 0), \
  (byte & 0x01 ? 1 : 0)


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
      case SingleBit:edit_->setInputMask("B"); break;
      case Bitfield:edit_->setInputMask("BBBBBBBB"); break;
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

      char buffer[11] = { 0 };

      switch (reg_type_)
      {
      case _8Bit:
         sprintf(buffer, "%2.2X", *value_);
         break;
      case _16Bit:
         sprintf(buffer, "%4.4X", *value_);
         break;
      case _ValueInt:
         sprintf(buffer, "%i", *value_);
         break;
      case SingleBit:
         sprintf(buffer, "%1.1X", ((*((unsigned char*)value_) & 0x1)));
         break;
      case Bitfield:
         sprintf(buffer, BYTETOBINARYPATTERN, BYTETOBINARY(*value_));
         break;
      }
      edit_->setText(buffer);
      edit_->setReadOnly(is_running);

      old_value_ = *value_;
   }

   virtual void Validate(bool is_running)
   {
      bool ok = true;;

      T value = 0;
      switch (reg_type_)
      {
      case _8Bit:
      case _16Bit:
         value = static_cast<T> (edit_->text().toUInt(&ok, 16));
         break;
      case _ValueInt:
         value = static_cast<T> (edit_->text().toUInt(&ok, 10));
         break;
      case SingleBit:
      {
         auto txt = edit_->text().toStdString();
         if (txt == "0") { value = 0; ok = true; }
         else if (txt == "1") { value = 1; ok = true; }
         else ok = false;
         break;
      }
      case Bitfield:
      {
         auto txt = edit_->text().toStdString();
         for (int i = 0; i < 8; i++)
         {
            value <<= 1;
            value |= (txt[i] - 0x30);
         }
         break;
      }
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

