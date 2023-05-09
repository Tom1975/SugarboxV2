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

protected:

   // Listeners
   std::vector<ISettingsListener*> listeners_;

   // Values
   std::map<unsigned int, QColor > color_list_;
   std::map<unsigned int, QFont> font_list_;
   std::map<unsigned int, Action> action_list_;

};
