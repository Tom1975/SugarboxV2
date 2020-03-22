#pragma once

#include "ConfigurationManager.h"
#include "SettingsList.h"
#include "Machine.h"

class DlgSettings
{
public:
   DlgSettings(ConfigurationManager* config_manager);
   virtual ~DlgSettings();

   void Init(EmulatorEngine* engine);
   void Refresh(const char* path);
   void DisplayMenu();
   void DisplayConfigCombo();

protected:
   std::string conf_path_;
   ConfigurationManager *config_manager_;
   SettingsList settings_list_;

   MachineSettings* seletected_conf_;
   EmulatorEngine* engine_;
};

