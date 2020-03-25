#include "DlgSettings.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "ImGuiFileDialog/ImGuiFileDialog.h"

DlgSettings::DlgSettings(ConfigurationManager* config_manager) : config_manager_(config_manager), engine_(nullptr), seletected_conf_(nullptr)
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
      
   ImGui::SetNextItemWidth(300.0f);
   if (ImGui::BeginCombo("##Configuration", current_conf))
   {

      for (int i = 0; i < nb_conf; i++)
      {
         MachineSettings* conf = settings_list_.GetConfiguration(i);
         if (ImGui::Selectable(conf->GetShortDescription(), seletected_conf_ == conf))
         {
            // Set it !
            conf->Load();
            engine_->ChangeConfig(conf);
            engine_->OnOff();
            seletected_conf_ = conf;
         }
      }
      ImGui::EndCombo();
   }
}

void DlgSettings::DisplayMenu()
{
   ImGui::Begin("Configuration");

   // Display contents in a scrolling region
   ImGui::TextColored(ImVec4(1, 1, 0, 1), "Important Stuff");
   ImGui::BeginChild("Scrolling", ImVec2(250, 0), true);
   int nb_conf = settings_list_.GetNumberOfConfigurations();

   for (int i = 0; i < nb_conf; i++)
   {
      MachineSettings* conf = settings_list_.GetConfiguration(i);

      if (ImGui::Selectable(conf->GetShortDescription(), seletected_conf_ == conf))
      {
         // Current setting selection
         seletected_conf_ = conf;
      }
   }
   ImGui::EndChild();
   ImGui::SameLine();
   ImGui::BeginGroup();
      // Display configuration list
      if (seletected_conf_ != nullptr)
      {
         // Hardware type
         // CRTC
         // RAM quatity
         // ROM /Cartridge
         // Keyboard type
      }
   ImGui::EndGroup();

   ImGui::End();

}