#include "Settings.h"

#include <cstring>


#include "ConfigurationManager.h"


/////////////////////////////////////////////////////////////
// Default definitions


/////////////////////////////////////////////////////////////
/// Helper functions
Settings::Settings()
{
   color_list_ =
   {
      {BACK_COLOR, Qt::white, },
      {MARGIN_COLOR, QColor(220, 220, 220)},
      {ADDRESS_COLOR,Qt::blue},
      {MNEMONIC_COLOR,Qt::darkBlue},
      {ARGUMENT_COLOR,Qt::darkMagenta},
      {BYTE_COLOR,Qt::darkGray},
      {CHAR_SOLOR,Qt::gray},
      {SELECTION_COLOR, QColor(128, 128, 255, 128)},
   };

   /*font_list_ =
   {
      {DISASSEMBLY_FONT, QFont()},
      {REGISTER_FONT, },
      {MEMORY_FONT, },

   };*/

   action_list_ =
   {
      {DBG_TOGGLE_BREAKPOINT_ACTION, {"TGL_BKP", Qt::Key_F9, }},
   };
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


QColor Settings::GetColor(ColorType id)
{
   return color_list_[id];
}

QFont Settings::GetFont(FontType id)
{
   return font_list_[id];
}

Action Settings::GetAction(ActionType id)
{
   return action_list_[id];
}