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

class Action
{
public:
   std::string label_;
   Qt::Key shortcut_;
   std::function<void(void)> function_;
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

   enum ActionType
   {
      DBG_BREAK_ACTION,
      DBG_STEP_ACTION,
      DBG_STEPIN_ACTION,
      DBG_STEPOUT_ACTION,
      DBG_TOGGLE_FLAG_ACTION,
      DBG_TOGGLE_BREAKPOINT_ACTION,

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
   Action GetAction(ActionType);

protected:

   // Listeners
   std::vector<ISettingsListener*> listeners_;

   // Values
   std::map<ColorType, QColor > color_list_;
   std::map<FontType, QFont> font_list_;
   std::map<ActionType, Action> action_list_;

};
