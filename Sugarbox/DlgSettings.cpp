#include "DlgSettings.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "ImGuiFileDialog/ImGuiFileDialog.h"

DlgSettings::DlgSettings(ConfigurationManager* config_manager) : config_manager_(config_manager), seletected_conf_(nullptr)
{
}

DlgSettings::~DlgSettings()
{

}

void DlgSettings::Refresh(const char* path)
{
   conf_path_ = path;
   settings_list_.InitSettingsList(config_manager_, conf_path_.c_str());

   settings_list_.BuildList();
}


void DlgSettings::DisplayMenu()
{
   ImGui::Begin("Configuration");

   // Display contents in a scrolling region
   ImGui::TextColored(ImVec4(1, 1, 0, 1), "Important Stuff");
   ImGui::BeginChild("Scrolling");
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

   // Display configuration list
   if (seletected_conf_ != nullptr)
   {

   }

   ImGui::End();

}