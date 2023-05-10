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


   Action(const char* label = "", Qt::Key shortcut = Qt::Key_Escape, Qt::KeyboardModifier modifiers = Qt::NoModifier) :
      label_(label), shortcut_(shortcut), modifiers_(modifiers)
   {

   }

   std::string label_;
   Qt::Key shortcut_;
   Qt::KeyboardModifier modifiers_;
};

class Settings
{
public:

   Settings();
   virtual ~Settings();

   // Value used


   // Creation / Modify / Update 
   void AddColor(unsigned int , QColor);
   void AddFont(unsigned int, QFont);
   void AddAction(unsigned int, Action);

   // Serialization
   void Load(const char* path);
   void Save(const char* path);
   
   // Register listener
   void RegisterListener(ISettingsListener*);


   // Access to values
   QColor GetColor(unsigned int);
   QFont GetFont(unsigned int);
   Action GetAction(unsigned int);
   unsigned int GetActionShortcut(unsigned int);

protected:

   // Listeners
   std::vector<ISettingsListener*> listeners_;

   // Values
   std::map<unsigned int, QColor > color_list_;
   std::map<unsigned int, QFont> font_list_;
   std::map<unsigned int, Action> action_list_;

};
