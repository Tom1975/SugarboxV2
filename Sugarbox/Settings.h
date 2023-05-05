#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>

#include <QWidget>

class ISettingsListener
{
public:
   virtual void SettingsHadChanged() = 0;
};

class Settings
{
public:

   Settings();
   virtual ~Settings();

   // Value used
   enum ColorType
   {
      BACK_COLOR,
      MARGIN_COLOR,
      ADDRESS_COLOR,
      MNEMONIC_COLOR,
      ARGUMENT_COLOR,
      BYTE_COLOR,
      CHAR_SOLOR,
      SELECTION_COLOR
   };

   enum FontType
   {
      DISASSEMBLY_FONT,
      REGISTER_FONT,
      MEMORY_FONT,
   };

   // Serialization
   void Load(const char* path);
   void Save(const char* path);
   
   // Register listener
   void RegisterListener(ISettingsListener*);

   // Modify / update 
   // todo

   // Access to values
   QColor GetColor(ColorType);
   QFont GetFont(FontType);

protected:

   // Listeners
   std::vector<ISettingsListener*> listeners_;

   // Values

   class Action
   {
   public:
      std::string label_;
      Qt::Key shortcut_;
      std::function<void(void)> function_;
   };

   std::map<ColorType, QColor > color_list_;
   std::map<unsigned int, QFont> font_list_;
   std::map<unsigned int, Action> action_list_;

};
