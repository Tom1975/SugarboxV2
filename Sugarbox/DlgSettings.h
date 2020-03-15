#pragma once

#include "ConfigurationManager.h"
#include "SettingsList.h"

class DlgSettings
{
public:
   DlgSettings(ConfigurationManager* config_manager);
   virtual ~DlgSettings();

   void Refresh(const char* path);
   void DisplayMenu();

protected:
   std::string conf_path_;
   ConfigurationManager *config_manager_;
   SettingsList settings_list_;

   MachineSettings* seletected_conf_;
};

