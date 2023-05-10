#include "Settings.h"

#include <cstring>


#include "ConfigurationManager.h"


/////////////////////////////////////////////////////////////
// Default definitions


/////////////////////////////////////////////////////////////
/// Helper functions
Settings::Settings()
{
}

Settings::~Settings()
{
}

void Settings::Load(const char* path)
{
   ConfigurationManager config_manager;
   config_manager.OpenFile(path);

   // Read all known configuration


   config_manager.CloseFile();
}

void Settings::Save(const char* path)
{
   ConfigurationManager config_manager;
   config_manager.OpenNewFile(path);

   // Save all known configuration
   

   config_manager.CloseFile();
}


void Settings::RegisterListener(ISettingsListener* listener)
{
   listeners_.push_back(listener);
}

void Settings::AddColor(unsigned int id, QColor color)
{
   color_list_[id] = color;
}

void Settings::AddFont(unsigned int id, QFont font)
{
   font_list_[id] = font;
}

void Settings::AddAction(unsigned int id, Action action)
{
   action_list_[id] = action;
}

QColor Settings::GetColor(unsigned int  id)
{
   return color_list_[id];
}

QFont Settings::GetFont(unsigned int  id)
{
   return font_list_[id];
}

Action Settings::GetAction(unsigned int  id)
{
   return action_list_[id];
}
unsigned int Settings::GetActionShortcut(unsigned int id)
{
   return ((action_list_[id].shortcut_) | (action_list_[id].modifiers_));
}