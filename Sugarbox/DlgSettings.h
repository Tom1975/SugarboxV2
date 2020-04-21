#pragma once

#include <QComboBox>
#include <QWidget>
#include "ConfigurationManager.h"
#include "SettingsList.h"
#include "Machine.h"

class ISettingsChange
{
public:
   virtual void ChangeSettings(MachineSettings*) = 0;
};


class DlgSettings : public QWidget
{
   Q_OBJECT

public:
   DlgSettings(ConfigurationManager* config_manager, ISettingsChange* parent);
   virtual ~DlgSettings();

   void Init(EmulatorEngine* engine);
   void Refresh(const char* path);
   void DisplayMenu();
   void DisplayConfigCombo();

   void UpdateCombo(QComboBox *config_box);

public slots:
   void ChangeSettings(int);

protected:
   std::string conf_path_;
   ConfigurationManager *config_manager_;
   SettingsList settings_list_;

   MachineSettings* seletected_conf_;
   EmulatorEngine* engine_;
   ISettingsChange* parent_;
};

