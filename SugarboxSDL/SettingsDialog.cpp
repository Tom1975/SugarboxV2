#include "SettingsDialog.h"

#include <algorithm>
#include <filesystem>

#include "imgui.h"

#include "ConfigurationManager.h"
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

   // As the files of CONF name them : the ROM of no slot is "No ROM"
   const char* kNoRom = "No ROM";

   struct Choice
   {
      int value;
      const char* label;
   };
   const Choice kModels[] = {
      { MachineSettings::OLD_464, "CPC 464" },
      { MachineSettings::OLD_664, "CPC 664" },
      { MachineSettings::OLD_6128, "CPC 6128" },
      { MachineSettings::PLUS_464, "464 Plus" },
      { MachineSettings::PLUS_6128, "6128 Plus" },
      { MachineSettings::GX400, "GX4000" },
   };
   const Choice kCrtcs[] = {
      { CRTC::HD6845S, "0 - HD6845S / UM6845" },
      { CRTC::UM6845R, "1 - UM6845R" },
      { CRTC::MC6845, "2 - MC6845" },
      { CRTC::AMS40489, "3 - AMS40489 (Plus ASIC)" },
      { CRTC::AMS40226, "4 - AMS40226 (pre-ASIC)" },
   };
   const Choice kRams[] = {
      { MachineSettings::M64_K, "64 KB" },
      { MachineSettings::M128_K, "128 KB" },
      { MachineSettings::M512_K, "64 KB + 512 KB expansion" },
   };

   // Combo of a fixed list : true when the value changed
   template <size_t N>
   bool ChoiceCombo(const char* id, int& value, const Choice (&choices)[N])
   {
      const char* current = "?";
      for (const auto& choice : choices)
      {
         if (choice.value == value) current = choice.label;
      }
      bool changed = false;
      if (ImGui::BeginCombo(id, current))
      {
         for (const auto& choice : choices)
         {
            if (ImGui::Selectable(choice.label, choice.value == value) && choice.value != value)
            {
               value = choice.value;
               changed = true;
            }
         }
         ImGui::EndCombo();
      }
      return changed;
   }

   // A file name made of what can be typed for a machine name
   std::string FileNameFrom(const std::string& name)
   {
      std::string file;
      for (const char c : name)
      {
         if (isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-') file += c;
         else if (c == ' ' && !file.empty() && file.back() != '_') file += '_';
      }
      return file;
   }

   void Label(const char* text)
   {
      ImGui::AlignTextToFramePadding();
      ImGui::TextUnformatted(text);
      ImGui::SameLine(ImGui::GetFontSize() * 10.0f);
      ImGui::SetNextItemWidth(-FLT_MIN);
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

   // The machine running, ready to be changed
   ScanFiles();
   machine_.reset();
   machine_dirty_ = false;
   SettingsList* machines = host_->GetMachines();
   for (unsigned int i = 0; machines != nullptr && i < machines->GetNumberOfConfigurations(); i++)
   {
      MachineSettings* machine = machines->GetConfiguration(i);
      if (machine != nullptr && host_->IsCurrentMachine(*machine))
      {
         SelectMachine(machine->GetFilePath());
      }
   }
}

void SettingsDialog::Apply()
{
   if (machine_dirty_) SaveMachine();
   host_->GetSettings() = edited_;
   host_->ApplySettings();
}

bool SettingsDialog::Ask(const char* message, const char* yes, const char* no)
{
   const SDL_MessageBoxButtonData buttons[] = {
      { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, no },
      { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, yes } };
   SDL_MessageBoxData data = {};
   data.flags = SDL_MESSAGEBOX_WARNING;
   data.window = window_;
   data.title = "Sugarbox";
   data.message = message;
   data.numbuttons = 2;
   data.buttons = buttons;
   int button = 0;
   return SDL_ShowMessageBox(&data, &button) && button == 1;
}

void SettingsDialog::ShowPathDialog(PathKind kind, const char* default_location)
{
   // Freed by the callback, which is always called once
   auto* request = new PathRequest{ this, kind };
   static const SDL_DialogFileFilter cartridge_filters[] = { { "Cartridge", "cpr;bin" }, { "All files", "*" } };
   switch (kind)
   {
   case PATH_SCREENSHOT_FOLDER:
      SDL_ShowOpenFolderDialog(PathCallback, request, window_, default_location, false);
      break;
   case PATH_CARTRIDGE:
      SDL_ShowOpenFileDialog(PathCallback, request, window_, cartridge_filters, 2, default_location, false);
      break;
   }
}

void SDLCALL SettingsDialog::PathCallback(void* userdata, const char* const* filelist, int)
{
   PathRequest* request = static_cast<PathRequest*>(userdata);
   if (filelist != nullptr && filelist[0] != nullptr)
   {
      std::lock_guard<std::mutex> lock(request->dialog->path_mutex_);
      request->dialog->path_results_.emplace_back(request->kind, filelist[0]);
   }
   delete request;
}

void SettingsDialog::HandlePathResults()
{
   std::vector<std::pair<PathKind, std::string>> results;
   {
      std::lock_guard<std::mutex> lock(path_mutex_);
      results.swap(path_results_);
   }
   for (const auto& result : results)
   {
      switch (result.first)
      {
      case PATH_SCREENSHOT_FOLDER:
         edited_.screenshot_dir = result.second;
         break;
      case PATH_CARTRIDGE:
         if (machine_)
         {
            // Relative to the base directory when it is inside, as the files of CONF name it
            std::error_code error;
            std::filesystem::path path = std::filesystem::u8path(result.second);
            const std::filesystem::path relative = std::filesystem::relative(path, std::filesystem::current_path(), error);
            if (!error && !relative.empty() && relative.native().rfind(std::filesystem::path("..").native(), 0) != 0)
            {
               path = relative;
            }
            machine_->SetDefaultCartridge(path.generic_u8string().c_str());
            machine_dirty_ = true;
         }
         break;
      }
   }
}

bool SettingsDialog::CaptureKey(SDL_Scancode code)
{
   if (!open_ || capture_joystick_ < 0) return false;
   // Escape : no change
   if (code != SDL_SCANCODE_ESCAPE)
   {
      edited_.joysticks[capture_joystick_].keys[capture_key_] = code;
   }
   capture_joystick_ = -1;
   capture_key_ = -1;
   return true;
}

void SettingsDialog::Draw()
{
   if (!open_)
   {
      capture_joystick_ = -1;
      return;
   }

   HandlePathResults();

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
         if (ImGui::BeginTabItem("Machine"))
         {
            DrawMachineTab();
            ImGui::EndTabItem();
         }
         if (ImGui::BeginTabItem("Emulator"))
         {
            DrawEmulatorTab();
            ImGui::EndTabItem();
         }
         if (ImGui::BeginTabItem("Display"))
         {
            DrawDisplayTab();
            ImGui::EndTabItem();
         }
         if (ImGui::BeginTabItem("Input"))
         {
            DrawInputTab();
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
      if (ConfirmDiscardMachine()) open_ = false;
   }
   // Closed with its title bar button
   if (!open_ && machine_dirty_ && !ConfirmDiscardMachine()) open_ = true;
   if (!open_) capture_joystick_ = -1;

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
      ShowPathDialog(PATH_SCREENSHOT_FOLDER, folder.c_str());
   }
   ImGui::SameLine();
   if (ImGui::Button("Default", ImVec2(browse_width, 0.0f))) edited_.screenshot_dir.clear();
}

///////////////////////////////////////////////////
// Display tab

void SettingsDialog::DrawDisplayTab()
{
   ImGui::SeparatorText("Image");
   static const Choice scalings[] = {
      { AppSettings::SCALE_FIT, "Fit the window, proportions kept" },
      { AppSettings::SCALE_INTEGER, "Whole multiples only (sharp pixels)" },
      { AppSettings::SCALE_STRETCH, "Stretch to the window" },
   };
   int scaling = edited_.scaling;
   Label("Scaling");
   if (ChoiceCombo("##scaling", scaling, scalings)) edited_.scaling = static_cast<AppSettings::Scaling>(scaling);

   static const Choice filters[] = { { 0, "Sharp (nearest)" }, { 1, "Smooth (linear)" } };
   int smooth = edited_.smooth ? 1 : 0;
   Label("Filter");
   if (ChoiceCombo("##filter", smooth, filters)) edited_.smooth = smooth != 0;

   ImGui::Checkbox("Scanlines", &edited_.scanlines);
   ImGui::BeginDisabled(!edited_.scanlines);
   Label("Brightness");
   ImGui::SliderInt("##scanline", &edited_.scanline_brightness, 0, 100, "%d %%", ImGuiSliderFlags_AlwaysClamp);
   ImGui::EndDisabled();

   ImGui::SeparatorText("Window");
   ImGui::Checkbox("Start in full screen", &edited_.start_full_screen);
   ImGui::Checkbox("Status bar", &edited_.show_status_bar);
   ImGui::TextDisabled("The size and the position of the window are kept at exit.");
}

///////////////////////////////////////////////////
// Input tab

void SettingsDialog::DrawInputTab()
{
   static const Choice sources[] = {
      { AppSettings::JOY_NONE, "None" },
      { AppSettings::JOY_GAMEPAD, "Gamepad" },
      { AppSettings::JOY_KEYBOARD, "Keyboard" },
   };
   static const char* key_labels[AppSettings::KEY_COUNT] = { "Up", "Down", "Left", "Right", "Fire 1", "Fire 2" };
   const std::vector<std::string> gamepads = host_->GetGamepadNames();

   for (int joy = 0; joy < 2; joy++)
   {
      AppSettings::Joystick& joystick = edited_.joysticks[joy];
      ImGui::PushID(joy);
      ImGui::SeparatorText(joy == 0 ? "CPC joystick 1" : "CPC joystick 2");
      if (joy == 1)
      {
         // KeyboardHandler::JoystickAction only wires the first one
         ImGui::TextDisabled("Not emulated yet.");
         ImGui::PopID();
         continue;
      }

      int source = joystick.source;
      Label("Source");
      if (ChoiceCombo("##source", source, sources)) joystick.source = static_cast<AppSettings::JoystickSource>(source);

      if (joystick.source == AppSettings::JOY_GAMEPAD)
      {
         Label("Gamepad");
         const char* current = joystick.gamepad.empty() ? "First gamepad plugged in" : joystick.gamepad.c_str();
         if (ImGui::BeginCombo("##gamepad", current))
         {
            if (ImGui::Selectable("First gamepad plugged in", joystick.gamepad.empty())) joystick.gamepad.clear();
            for (const auto& name : gamepads)
            {
               if (ImGui::Selectable(name.c_str(), name == joystick.gamepad)) joystick.gamepad = name;
            }
            ImGui::EndCombo();
         }
         if (gamepads.empty()) ImGui::TextDisabled("No gamepad plugged in.");
         else if (!joystick.gamepad.empty() && std::find(gamepads.begin(), gamepads.end(), joystick.gamepad) == gamepads.end())
         {
            ImGui::TextDisabled("This gamepad is not plugged in.");
         }
         ImGui::TextDisabled("D-pad or left stick ; A / X : fire 1 ; B / Y : fire 2.");
      }
      else if (joystick.source == AppSettings::JOY_KEYBOARD)
      {
         ImGui::TextDisabled("These keys no longer go to the CPC keyboard.");
         for (int key = 0; key < AppSettings::KEY_COUNT; key++)
         {
            ImGui::PushID(key);
            Label(key_labels[key]);
            const bool waiting = capture_joystick_ == joy && capture_key_ == key;
            const char* name = SDL_GetScancodeName(joystick.keys[key]);
            if (ImGui::Button(waiting ? "Press a key (Escape : cancel)" : (name[0] != 0 ? name : "?"), ImVec2(ImGui::GetFontSize() * 16.0f, 0.0f)))
            {
               capture_joystick_ = joy;
               capture_key_ = key;
            }
            ImGui::PopID();
         }
      }
      ImGui::PopID();
   }

   ImGui::SeparatorText("Shortcuts");
   ImGui::TextUnformatted("Alt+Enter : full screen\nF12 : screenshot");
}

///////////////////////////////////////////////////
// Machine tab

void SettingsDialog::ScanFiles()
{
   // ROM files
   roms_.clear();
   std::error_code error;
   for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::current_path() / "ROM", error))
   {
      if (entry.is_regular_file(error)) roms_.push_back(entry.path().filename().u8string());
   }
   std::sort(roms_.begin(), roms_.end(), [](const std::string& a, const std::string& b) {
      return SDL_strcasecmp(a.c_str(), b.c_str()) < 0; });

   // CPC keyboards : the sections of KeyboardMaps.ini
   keyboards_.clear();
   ConfigurationManager keyboard_maps;
   keyboard_maps.OpenFile((std::filesystem::current_path() / "CONF" / "KeyboardMaps.ini").string().c_str());
   for (const char* section = keyboard_maps.GetFirstSection(); section != nullptr; section = keyboard_maps.GetNextSection())
   {
      keyboards_.push_back(section);
   }
}

void SettingsDialog::SelectMachine(const std::string& path)
{
   machine_.reset();
   machine_dirty_ = false;
   SettingsList* machines = host_->GetMachines();
   for (unsigned int i = 0; machines != nullptr && i < machines->GetNumberOfConfigurations(); i++)
   {
      MachineSettings* machine = machines->GetConfiguration(i);
      if (machine != nullptr && SameFile(machine->GetFilePath(), path))
      {
         // A copy : the list keeps what is in the file until it is saved
         machine_ = std::make_unique<MachineSettings>(*machine);
         machine_->Load(true);
         break;
      }
   }
}

bool SettingsDialog::ConfirmDiscardMachine()
{
   if (!machine_dirty_ || !machine_) return true;
   if (!Ask((std::string("The machine \"") + machine_->GetShortDescription() + "\" was changed.\nDiscard the changes ?").c_str(),
      "Discard", "Keep"))
   {
      return false;
   }
   machine_dirty_ = false;
   return true;
}

bool SettingsDialog::SaveMachine()
{
   if (!machine_) return false;
   const std::string path = machine_->GetFilePath();
   if (!machine_->Save())
   {
      SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Sugarbox", ("The machine could not be saved :\n" + path).c_str(), window_);
      return false;
   }
   machine_dirty_ = false;
   const bool running = host_->IsCurrentMachine(*machine_);
   host_->ReloadMachines();
   SelectMachine(path);

   if (running && Ask("This machine is the one running.\nRestart it now with the new settings ?", "Restart", "Later"))
   {
      host_->ChangeMachine(path);
   }
   return true;
}

void SettingsDialog::DuplicateMachine(const std::string& name)
{
   const std::string file = FileNameFrom(name);
   const std::filesystem::path path = std::filesystem::path("CONF") / (file + ".cfg");
   std::error_code error;
   if (file.empty())
   {
      new_name_error_ = "Type a name.";
      return;
   }
   if (std::filesystem::exists(path, error))
   {
      new_name_error_ = path.generic_string() + " already exists.";
      return;
   }

   MachineSettings copy(*machine_);
   copy.SetShortDescription(name.c_str());
   if (!copy.Save(path.generic_string().c_str()))
   {
      new_name_error_ = "The file could not be written.";
      return;
   }
   new_name_error_.clear();
   machine_dirty_ = false;
   host_->ReloadMachines();
   SelectMachine(path.generic_string());
   ImGui::CloseCurrentPopup();
}

void SettingsDialog::DeleteMachine()
{
   if (host_->IsCurrentMachine(*machine_))
   {
      SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Sugarbox",
         "This machine is the one running : choose another one in the Machine menu first.", window_);
      return;
   }
   const std::string path = machine_->GetFilePath();
   if (!Ask((std::string("Delete the machine \"") + machine_->GetShortDescription() + "\" ?\n" + path).c_str(), "Delete", "Cancel"))
   {
      return;
   }
   std::error_code error;
   std::filesystem::remove(std::filesystem::u8path(path), error);
   if (error)
   {
      SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Sugarbox", ("The file could not be deleted :\n" + path).c_str(), window_);
      return;
   }
   machine_.reset();
   machine_dirty_ = false;
   host_->ReloadMachines();
}

void SettingsDialog::DrawMachineTab()
{
   const float font = ImGui::GetFontSize();

   // List of the machines
   if (ImGui::BeginChild("##machines", ImVec2(font * 13.0f, 0.0f), ImGuiChildFlags_Borders))
   {
      SettingsList* machines = host_->GetMachines();
      for (unsigned int i = 0; machines != nullptr && i < machines->GetNumberOfConfigurations(); i++)
      {
         MachineSettings* machine = machines->GetConfiguration(i);
         if (machine == nullptr) continue;
         const bool selected = machine_ && SameFile(machine->GetFilePath(), machine_->GetFilePath());
         std::string label = machine->GetShortDescription();
         if (label.empty()) label = std::filesystem::u8path(machine->GetFilePath()).stem().u8string();
         if (host_->IsCurrentMachine(*machine)) label += "  (running)";
         ImGui::PushID(static_cast<int>(i));
         if (ImGui::Selectable(label.c_str(), selected) && !selected && ConfirmDiscardMachine())
         {
            SelectMachine(machine->GetFilePath());
         }
         ImGui::PopID();
      }
   }
   ImGui::EndChild();

   ImGui::SameLine();
   if (ImGui::BeginChild("##machine"))
   {
      if (machine_) DrawMachineDetail();
      else ImGui::TextDisabled("Select a machine.");
   }
   ImGui::EndChild();
}

void SettingsDialog::DrawMachineDetail()
{
   const float font = ImGui::GetFontSize();
   MachineSettings& machine = *machine_;
   char buffer[1024];

   // Actions
   ImGui::BeginDisabled(!machine_dirty_);
   if (ImGui::Button("Save")) SaveMachine();
   ImGui::SameLine();
   if (ImGui::Button("Revert")) SelectMachine(machine.GetFilePath());
   ImGui::EndDisabled();
   if (!machine_) return;
   ImGui::SameLine();
   if (ImGui::Button("Duplicate..."))
   {
      SDL_snprintf(new_name_, sizeof(new_name_), "%s copy", machine.GetShortDescription());
      new_name_error_.clear();
      ImGui::OpenPopup("Duplicate machine");
   }
   ImGui::SameLine();
   if (ImGui::Button("Delete...")) DeleteMachine();
   if (!machine_) return;

   if (ImGui::BeginPopupModal("Duplicate machine", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
   {
      ImGui::TextUnformatted("Name of the new machine :");
      ImGui::SetNextItemWidth(font * 20.0f);
      if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
      const bool enter = ImGui::InputText("##name", new_name_, sizeof(new_name_), ImGuiInputTextFlags_EnterReturnsTrue);
      ImGui::TextDisabled("File : CONF/%s.cfg", FileNameFrom(new_name_).c_str());
      if (!new_name_error_.empty()) ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", new_name_error_.c_str());
      if (ImGui::Button("Create") || enter) DuplicateMachine(new_name_);
      ImGui::SameLine();
      if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();
      ImGui::EndPopup();
   }

   ImGui::TextDisabled("%s", machine.GetFilePath());

   ImGui::SeparatorText("Description");
   SDL_strlcpy(buffer, machine.GetShortDescription(), sizeof(buffer));
   Label("Name");
   if (ImGui::InputText("##short", buffer, sizeof(buffer)))
   {
      machine.SetShortDescription(buffer);
      machine_dirty_ = true;
   }
   SDL_strlcpy(buffer, machine.GetDescription(), sizeof(buffer));
   Label("Description");
   if (ImGui::InputText("##desc", buffer, sizeof(buffer)))
   {
      machine.SetDescription(buffer);
      machine_dirty_ = true;
   }

   ImGui::SeparatorText("Hardware");
   int value = machine.GetHardwareType();
   Label("Model");
   if (ChoiceCombo("##model", value, kModels))
   {
      machine.SetHardwareType(static_cast<MachineSettings::HardwareType>(value));
      machine_dirty_ = true;
   }
   value = machine.GetCRTCType();
   Label("CRTC");
   if (ChoiceCombo("##crtc", value, kCrtcs))
   {
      machine.SetCRTCType(static_cast<CRTC::TypeCRTC>(value));
      machine_dirty_ = true;
   }
   value = machine.GetRamCfg();
   Label("RAM");
   if (ChoiceCombo("##ram", value, kRams))
   {
      machine.SetRamCfg(static_cast<MachineSettings::RamCfg>(value));
      machine_dirty_ = true;
   }
   bool flag = machine.FDCPlugged();
   if (ImGui::Checkbox("Disk drive (FDC)", &flag))
   {
      machine.SetFDCPlugged(flag);
      machine_dirty_ = true;
   }
   ImGui::SameLine();
   flag = machine.TapePlugged();
   if (ImGui::Checkbox("Tape player", &flag))
   {
      machine.SetTapePlugged(flag);
      machine_dirty_ = true;
   }
   ImGui::SameLine();
   flag = machine.PALPlugged();
   if (ImGui::Checkbox("PAL (RAM banking)", &flag))
   {
      machine.SetPALPlugged(flag);
      machine_dirty_ = true;
   }

   Label("CPC keyboard");
   const std::string keyboard = machine.GetKeyboardConfig();
   if (ImGui::BeginCombo("##keyboard", keyboard.c_str()))
   {
      for (const auto& name : keyboards_)
      {
         if (ImGui::Selectable(name.c_str(), name == keyboard) && name != keyboard)
         {
            machine.SetKeyboardConfig(name.c_str());
            machine_dirty_ = true;
         }
      }
      ImGui::EndCombo();
   }

   // ROMs : a combo of the files of the ROM directory
   ImGui::SeparatorText("ROM");
   auto rom_combo = [&](const char* id, const char* current, auto set) {
      const bool none = current == nullptr || current[0] == 0 || SDL_strcmp(current, kNoRom) == 0;
      if (ImGui::BeginCombo(id, none ? "-" : current, ImGuiComboFlags_HeightLarge))
      {
         if (ImGui::Selectable("-", none) && !none) set(nullptr);
         for (const auto& rom : roms_)
         {
            const bool selected = !none && rom == current;
            if (ImGui::Selectable(rom.c_str(), selected) && !selected) set(rom.c_str());
            if (selected) ImGui::SetItemDefaultFocus();
         }
         ImGui::EndCombo();
      }
   };
   Label("Lower ROM");
   rom_combo("##lower", machine.GetLowerRom(), [&](const char* rom) {
      machine.SetLowerRom(rom != nullptr ? rom : kNoRom);
      machine_dirty_ = true;
   });
   const int nb_roms = show_all_roms_ ? 256 : 16;
   for (int slot = 0; slot < nb_roms; slot++)
   {
      char label[32];
      SDL_snprintf(label, sizeof(label), "Upper ROM %d", slot);
      Label(label);
      ImGui::PushID(slot);
      rom_combo("##upper", machine.GetUpperRom(slot), [&](const char* rom) {
         // Slots 0 to 15 are written "No ROM" in the files of CONF, the others are left empty
         machine.SetUpperRom(slot, rom != nullptr ? rom : (slot < 16 ? kNoRom : ""));
         machine_dirty_ = true;
      });
      ImGui::PopID();
   }
   ImGui::Checkbox("Show the slots 16 to 255", &show_all_roms_);

   // Plus machines : the cartridge inserted at power on
   ImGui::SeparatorText("Cartridge (Plus, GX4000)");
   SDL_strlcpy(buffer, machine.GetDefaultCartridge(), sizeof(buffer));
   Label("Cartridge");
   ImGui::SetNextItemWidth(-(font * 5.0f + ImGui::GetStyle().ItemSpacing.x));
   if (ImGui::InputText("##cart", buffer, sizeof(buffer)))
   {
      machine.SetDefaultCartridge(buffer);
      machine_dirty_ = true;
   }
   ImGui::SameLine();
   if (ImGui::Button("Browse...", ImVec2(font * 5.0f, 0.0f)))
   {
      ShowPathDialog(PATH_CARTRIDGE, (std::filesystem::current_path() / "CART").u8string().c_str());
   }
}
