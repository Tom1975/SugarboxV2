#include "SettingsDialog.h"

#include <filesystem>

#include "imgui.h"

#include "SettingsList.h"

namespace
{
   // Speeds of the startup speed list, as written in Sugarbox.ini
   struct SpeedChoice
   {
      char limit;
      const char* label;
   };
   const SpeedChoice kSpeeds[] = {
      { 'G', "G-Sync / FreeSync (frames paced, sound follows)" },
      { 'S', "Sound" },
      { 'V', "Screen refresh (VSync)" },
      { 'F', "Maximum" },
      { 'C', "Custom percentage" },
   };

   // Path of a machine, as Sugarbox.ini stores it : relative to the current directory when it can
   std::string MachinePath(const char* path)
   {
      std::error_code error;
      const std::filesystem::path relative = std::filesystem::relative(std::filesystem::u8path(path), std::filesystem::current_path(), error);
      if (error || relative.empty() || relative.native().rfind(std::filesystem::path("..").native(), 0) == 0)
      {
         return path;
      }
      return relative.generic_u8string();
   }

   bool SameFile(const std::string& a, const std::string& b)
   {
      std::error_code error;
      return std::filesystem::path(a).filename() == std::filesystem::path(b).filename()
         && std::filesystem::weakly_canonical(a, error) == std::filesystem::weakly_canonical(b, error);
   }
}

void SettingsDialog::Init(SDL_Window* window, ISettingsHost* host)
{
   window_ = window;
   host_ = host;
}

void SettingsDialog::Open()
{
   edited_ = host_->GetSettings();
   open_ = true;
   just_opened_ = true;
}

void SettingsDialog::Apply()
{
   host_->GetSettings() = edited_;
   host_->ApplySettings();
}

void SDLCALL SettingsDialog::FolderCallback(void* userdata, const char* const* filelist, int)
{
   SettingsDialog* dialog = static_cast<SettingsDialog*>(userdata);
   if (filelist != nullptr && filelist[0] != nullptr)
   {
      std::lock_guard<std::mutex> lock(dialog->folder_mutex_);
      dialog->chosen_folder_ = filelist[0];
      dialog->folder_chosen_ = true;
   }
}

void SettingsDialog::Draw()
{
   if (!open_) return;

   {
      std::lock_guard<std::mutex> lock(folder_mutex_);
      if (folder_chosen_)
      {
         edited_.screenshot_dir = chosen_folder_;
         folder_chosen_ = false;
      }
   }

   const ImGuiViewport* viewport = ImGui::GetMainViewport();
   const float font = ImGui::GetFontSize();
   if (just_opened_)
   {
      ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
      ImGui::SetNextWindowFocus();
      just_opened_ = false;
   }
   ImGui::SetNextWindowSize(ImVec2(font * 46.0f, font * 30.0f), ImGuiCond_FirstUseEver);
   if (!ImGui::Begin("Settings", &open_, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings))
   {
      ImGui::End();
      return;
   }

   // Tabs above, buttons below
   const float buttons_height = ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y;
   if (ImGui::BeginChild("##tabs", ImVec2(0.0f, -buttons_height)))
   {
      if (ImGui::BeginTabBar("##settings"))
      {
         if (ImGui::BeginTabItem("Emulator"))
         {
            DrawEmulatorTab();
            ImGui::EndTabItem();
         }
         ImGui::EndTabBar();
      }
   }
   ImGui::EndChild();

   ImGui::Separator();
   const float button_width = font * 6.0f;
   ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - button_width * 3.0f - ImGui::GetStyle().ItemSpacing.x * 2.0f);
   if (ImGui::Button("OK", ImVec2(button_width, 0.0f)))
   {
      Apply();
      open_ = false;
   }
   ImGui::SameLine();
   if (ImGui::Button("Apply", ImVec2(button_width, 0.0f))) Apply();
   ImGui::SameLine();
   if (ImGui::Button("Cancel", ImVec2(button_width, 0.0f)) || (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsKeyPressed(ImGuiKey_Escape)))
   {
      open_ = false;
   }

   ImGui::End();
}

void SettingsDialog::DrawEmulatorTab()
{
   const float label_width = ImGui::GetFontSize() * 12.0f;
   auto label = [&](const char* text) {
      ImGui::AlignTextToFramePadding();
      ImGui::TextUnformatted(text);
      ImGui::SameLine(label_width);
      ImGui::SetNextItemWidth(-FLT_MIN);
   };

   ImGui::SeparatorText("Startup");

   // Machine : the machines of the CONF directory
   SettingsList* machines = host_->GetMachines();
   std::string current_name = edited_.startup_machine;
   for (unsigned int i = 0; machines != nullptr && i < machines->GetNumberOfConfigurations(); i++)
   {
      MachineSettings* machine = machines->GetConfiguration(i);
      if (machine != nullptr && SameFile(machine->GetFilePath(), edited_.startup_machine))
      {
         current_name = machine->GetShortDescription();
      }
   }
   label("Machine");
   if (ImGui::BeginCombo("##machine", current_name.c_str()))
   {
      for (unsigned int i = 0; machines != nullptr && i < machines->GetNumberOfConfigurations(); i++)
      {
         MachineSettings* machine = machines->GetConfiguration(i);
         if (machine == nullptr) continue;
         const bool selected = SameFile(machine->GetFilePath(), edited_.startup_machine);
         ImGui::PushID(static_cast<int>(i));
         if (ImGui::Selectable(machine->GetShortDescription(), selected))
         {
            edited_.startup_machine = MachinePath(machine->GetFilePath());
         }
         if (selected) ImGui::SetItemDefaultFocus();
         ImGui::PopID();
      }
      ImGui::EndCombo();
   }

   // Speed
   const char* speed_label = kSpeeds[0].label;
   for (const auto& choice : kSpeeds)
   {
      if (choice.limit == edited_.limit_speed) speed_label = choice.label;
   }
   label("Speed");
   if (ImGui::BeginCombo("##speed", speed_label))
   {
      for (const auto& choice : kSpeeds)
      {
         if (ImGui::Selectable(choice.label, choice.limit == edited_.limit_speed)) edited_.limit_speed = choice.limit;
      }
      ImGui::EndCombo();
   }
   if (edited_.limit_speed == 'C')
   {
      label("Percentage");
      ImGui::SliderInt("##percent", &edited_.speed, 10, 1000, "%d %%", ImGuiSliderFlags_AlwaysClamp);
   }
   ImGui::TextDisabled("The command line option --speed takes precedence.");

   ImGui::Checkbox("Insert again the disks left in the drives", &edited_.reload_disks);

   ImGui::SeparatorText("Sound");
   int volume = static_cast<int>(edited_.volume * 100.0f + 0.5f);
   label("Volume");
   if (ImGui::SliderInt("##volume", &volume, 0, 100, "%d %%", ImGuiSliderFlags_AlwaysClamp)) edited_.volume = volume / 100.0f;
   ImGui::Checkbox("Mute", &edited_.mute);

   ImGui::SeparatorText("Media");
   ImGui::Checkbox("Autoload (run the program of an inserted disk)", &edited_.autoload);

   ImGui::SeparatorText("Screenshots");
   label("Folder");
   const float browse_width = ImGui::GetFontSize() * 5.0f;
   const std::string folder = edited_.GetScreenshotFolder();
   char buffer[1024];
   SDL_strlcpy(buffer, folder.c_str(), sizeof(buffer));
   ImGui::SetNextItemWidth(-(browse_width * 2.0f + ImGui::GetStyle().ItemSpacing.x * 2.0f));
   if (ImGui::InputText("##folder", buffer, sizeof(buffer)))
   {
      edited_.screenshot_dir = buffer;
   }
   ImGui::SameLine();
   if (ImGui::Button("Browse...", ImVec2(browse_width, 0.0f)))
   {
      SDL_ShowOpenFolderDialog(FolderCallback, this, window_, folder.c_str(), false);
   }
   ImGui::SameLine();
   if (ImGui::Button("Default", ImVec2(browse_width, 0.0f))) edited_.screenshot_dir.clear();
}
