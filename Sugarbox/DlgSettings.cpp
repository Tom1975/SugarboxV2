#include "DlgSettings.h"
#include "SugarboxApp.h"

DlgSettings::DlgSettings(ConfigurationManager* config_manager, ISettingsChange* parent) : config_manager_(config_manager), engine_(nullptr), seletected_conf_(nullptr), parent_(parent)
{
}

DlgSettings::~DlgSettings()
{

}

void DlgSettings::Init(EmulatorEngine* engine)
{
   engine_ = engine;
   seletected_conf_ = engine_->GetSettings();
}

void DlgSettings::Refresh(const char* path)
{
   conf_path_ = path;
   settings_list_.InitSettingsList(config_manager_, conf_path_.c_str());

   settings_list_.BuildList();
}

void DlgSettings::DisplayConfigCombo()
{
   int nb_conf = settings_list_.GetNumberOfConfigurations();

   const char* current_conf = nullptr;
   if (seletected_conf_ != nullptr)
   {
      current_conf = seletected_conf_->GetShortDescription();
   }
      
}

void DlgSettings::DisplayMenu()
{


}

void DlgSettings::UpdateCombo(QComboBox *config_box)
{
   config_box->clear();
   int index_sel = 0;
   for (int i = 0; i < settings_list_.GetNumberOfConfigurations(); i++)
   {
      MachineSettings* settings = settings_list_.GetConfiguration(i);
      if (settings != nullptr)
      {
         const char* shortname = settings->GetShortDescription();

         config_box->addItem( shortname, i );

         if (*engine_->GetSettings() == *settings)
         {
            index_sel = i;
         }
      }
   }
   config_box->setCurrentIndex(index_sel);

   //connect(config_box, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index) { parent_->ChangeSettings(settings_list_.GetConfiguration(index)); });
   QObject::connect(config_box, SIGNAL(currentIndexChanged(int)), SLOT(ChangeSettings(int)));
}

void DlgSettings::ChangeSettings(int newindex)
{
   parent_->ChangeSettings(settings_list_.GetConfiguration(newindex));
}