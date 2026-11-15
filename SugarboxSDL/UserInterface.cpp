#include "UserInterface.h"

#include <algorithm>
#include <cstdio>
#include <filesystem>

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlgpu3.h"

#include "Emulation.h"
#include "ImageWriter.h"
#include "SDLSound.h"

namespace
{
   const SDL_DialogFileFilter kAllFiles = { "All files", "*" };

   const SDL_DialogFileFilter kScriptFilters[] = { { "CSL script", "csl" }, kAllFiles };
   const SDL_DialogFileFilter kTapeLoadFilters[] = { { "Tape dump", "wav;cdt;csw;tzx" }, kAllFiles };
   const SDL_DialogFileFilter kSnaFilters[] = { { "Snapshot", "sna" }, kAllFiles };
   const SDL_DialogFileFilter kSnrFilters[] = { { "Snapshot record", "snr" }, kAllFiles };
   const SDL_DialogFileFilter kCprFilters[] = { { "Cartridge", "cpr;bin;xpr" }, kAllFiles };
   const SDL_DialogFileFilter kPngFilters[] = { { "PNG image", "png" } };

   // Same order as the filters
   const SDL_DialogFileFilter kTapeSaveFilters[] = {
      { "WAV", "wav" }, { "CDT-drb", "cdt" }, { "CDT-CSW", "cdt" }, { "CSW 1.1", "csw" }, { "CSW 2.0", "csw" } };
   const Emulation::TapeFormat kTapeSaveFormats[] = {
      Emulation::TAPE_WAV, Emulation::TAPE_CDT_DRB, Emulation::TAPE_CDT_CSW, Emulation::TAPE_CSW11, Emulation::TAPE_CSW20 };
   const int kNbTapeSaveFormats = 5;

   // Speeds of the menu
   enum { SPEED_CUSTOM, SPEED_VSYNC, SPEED_SOUND, SPEED_GSYNC, SPEED_FULL };

   std::string LowerExtension(const std::string& path)
   {
      std::string extension = std::filesystem::u8path(path).extension().u8string();
      if (!extension.empty() && extension[0] == '.') extension.erase(0, 1);
      for (auto& c : extension) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
      return extension;
   }

   // First extension of a "a;b;c" pattern
   std::string FirstExtension(const char* pattern)
   {
      std::string extension = pattern;
      const size_t separator = extension.find(';');
      if (separator != std::string::npos) extension.erase(separator);
      return extension;
   }
}

bool UserInterface::Init(SDL_Window* window, SDLDisplay* display, Emulation* emulation, SDLSound* sound, IUserInterfaceHost* host, float scale)
{
   SDL_GPUDevice* device = display->GetDevice();
   window_ = window;
   display_ = display;
   emulation_ = emulation;
   sound_ = sound;
   host_ = host;

   IMGUI_CHECKVERSION();
   ImGui::CreateContext();
   ImGuiIO& io = ImGui::GetIO();
   // No keyboard navigation : the keyboard is the one of the emulated machine, unless a text is edited
   io.IniFilename = nullptr;
   ImGui::StyleColorsDark();
   ImGuiStyle& style = ImGui::GetStyle();
   style.ScaleAllSizes(scale);
   style.FontScaleDpi = scale;

   if (!ImGui_ImplSDL3_InitForSDLGPU(window))
   {
      return false;
   }
   ImGui_ImplSDLGPU3_InitInfo init_info = {};
   init_info.Device = device;
   init_info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(device, window);
   init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;
   if (!ImGui_ImplSDLGPU3_Init(&init_info))
   {
      return false;
   }
   initialised_ = true;

   // Hardware configurations
   const std::filesystem::path conf_path = std::filesystem::current_path() / "CONF";
   settings_list_.InitSettingsList(emulation_->GetConfigurationManager(), conf_path.string().c_str());
   settings_list_.BuildList();

   // Disk formats. The strings are all stored before the filters point to them.
   const std::vector<FormatType*> load_formats = disk_builder_.GetFormatsList(DiskBuilder::READ);
   disk_save_formats_ = disk_builder_.GetFormatsList(DiskBuilder::WRITE);

   std::string disk_pattern;
   for (const auto& format : load_formats)
   {
      if (!disk_pattern.empty()) disk_pattern += ";";
      disk_pattern += format->GetFormatExt();
   }
   media_pattern_ = disk_pattern + ";sna;snr;wav;cdt;csw;tzx;cpr;xpr;bin;zip;csl";

   filter_strings_.reserve(1 + disk_save_formats_.size() * 2);
   filter_strings_.push_back(disk_pattern);
   for (const auto& format : disk_save_formats_)
   {
      filter_strings_.push_back(format->GetFormatDescriptor());
      filter_strings_.push_back(format->GetFormatExt());
   }
   disk_load_filters_ = { { "Disk dump", filter_strings_[0].c_str() }, kAllFiles };
   for (size_t i = 0; i < disk_save_formats_.size(); i++)
   {
      disk_save_filters_.push_back({ filter_strings_[1 + i * 2].c_str(), filter_strings_[2 + i * 2].c_str() });
   }
   media_filters_ = { { "Media", media_pattern_.c_str() }, kAllFiles };
   return true;
}

void UserInterface::Shutdown()
{
   if (!initialised_) return;
   initialised_ = false;
   ImGui_ImplSDL3_Shutdown();
   ImGui_ImplSDLGPU3_Shutdown();
   ImGui::DestroyContext();
}

void UserInterface::ProcessEvent(const SDL_Event& event)
{
   if (initialised_) ImGui_ImplSDL3_ProcessEvent(&event);
}

bool UserInterface::WantsKeyboard() const
{
   return initialised_ && ImGui::GetIO().WantCaptureKeyboard;
}

///////////////////////////////////////////////////
// Rendering

void UserInterface::Prepare(SDL_GPUCommandBuffer* command_buffer)
{
   ImGui_ImplSDLGPU3_PrepareDrawData(ImGui::GetDrawData(), command_buffer);
}

void UserInterface::Draw(SDL_GPUCommandBuffer* command_buffer, SDL_GPURenderPass* render_pass)
{
   ImGui_ImplSDLGPU3_RenderDrawData(ImGui::GetDrawData(), command_buffer, render_pass);
}

void UserInterface::BuildFrame(const Statistics& statistics)
{
   ImGui_ImplSDLGPU3_NewFrame();
   ImGui_ImplSDL3_NewFrame();
   ImGui::NewFrame();

   const ImGuiIO& io = ImGui::GetIO();
   // In full screen, the bars are shown over the emulated screen, and only when the mouse is
   // at the top of the screen (or while a menu is open)
   const bool full_screen = host_->IsFullScreen();
   const bool mouse_at_top = io.MousePos.y >= 0.0f && io.MousePos.y < ImGui::GetFrameHeight() * 3.0f;
   const bool show_bars = !full_screen || popup_open_ || mouse_at_top;

   top_margin_ = 0;
   bottom_margin_ = 0;
   if (show_bars)
   {
      DrawMenuBar();
      DrawStatusBar(statistics);
      if (full_screen)
      {
         top_margin_ = 0;
         bottom_margin_ = 0;
      }
   }
   popup_open_ = ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel);

   ImGui::Render();
}

bool UserInterface::IsSpeed(int limit, int speed) const
{
   EmulatorEngine* engine = emulation_->GetEngine();
   switch (limit)
   {
   case SPEED_CUSTOM: return engine->GetSpeedLimit() == EmulatorEngine::E_CUSTOM && static_cast<int>(engine->GetSpeed()) == speed;
   case SPEED_VSYNC: return engine->GetSpeedLimit() == EmulatorEngine::E_VBL;
   case SPEED_SOUND: return engine->GetSpeedLimit() == EmulatorEngine::E_SOUND;
   case SPEED_GSYNC: return engine->GetSpeedLimit() == EmulatorEngine::E_SOUND_AND_VBL;
   default: return engine->GetSpeedLimit() == EmulatorEngine::E_FULL;
   }
}

void UserInterface::SetSpeed(int limit, int speed)
{
   const Emulation::CommandLock lock(emulation_);
   EmulatorEngine* engine = emulation_->GetEngine();
   switch (limit)
   {
   case SPEED_CUSTOM:
      engine->SetSpeedLimit(EmulatorEngine::E_CUSTOM);
      engine->SetSpeed(speed);
      break;
   case SPEED_VSYNC:
      engine->SetSpeedLimit(EmulatorEngine::E_VBL);
      engine->SetSpeed(100);
      break;
   case SPEED_SOUND:
      engine->SetSpeedLimit(EmulatorEngine::E_SOUND);
      engine->SetSpeed(100);
      break;
   case SPEED_GSYNC:
      engine->SetSpeedLimit(EmulatorEngine::E_SOUND_AND_VBL);
      engine->SetSpeed(100);
      break;
   default:
      engine->SetSpeedLimit(EmulatorEngine::E_FULL);
      break;
   }
}

void UserInterface::DrawDiskMenu(int drive)
{
   EmulatorEngine* engine = emulation_->GetEngine();
   const bool disk_present = emulation_->IsDiskPresent(drive);

   if (ImGui::MenuItem("Insert...")) OpenDialog(DLG_INSERT_DISK, drive);
   if (ImGui::BeginMenu("Insert blank disk"))
   {
      if (ImGui::MenuItem("Vendor format") && ConfirmDiskChange(drive)) emulation_->InsertBlankDisk(drive, IDisk::VENDOR);
      if (ImGui::MenuItem("Data format") && ConfirmDiskChange(drive)) emulation_->InsertBlankDisk(drive, IDisk::DATA);
      ImGui::EndMenu();
   }
   if (ImGui::MenuItem("Flip", nullptr, false, disk_present))
   {
      const Emulation::CommandLock lock(emulation_);
      engine->FlipDisk(drive);
   }
   if (ImGui::MenuItem("Eject", nullptr, false, disk_present) && ConfirmDiskChange(drive))
   {
      const Emulation::CommandLock lock(emulation_);
      engine->Eject(drive);
   }
   if (ImGui::MenuItem("Save as...", nullptr, false, disk_present)) OpenDialog(DLG_SAVE_DISK, drive);
}

void UserInterface::DrawMenuBar()
{
   if (!ImGui::BeginMainMenuBar()) return;
   top_margin_ = static_cast<int>(ImGui::GetWindowSize().y * ImGui::GetIO().DisplayFramebufferScale.y);

   EmulatorEngine* engine = emulation_->GetEngine();

   if (ImGui::BeginMenu("File"))
   {
      if (ImGui::MenuItem("Open media...")) OpenDialog(DLG_OPEN_MEDIA);
      if (ImGui::MenuItem("Run CSL script...")) OpenDialog(DLG_RUN_SCRIPT);
      ImGui::Separator();
      if (ImGui::MenuItem("Autoload", nullptr, emulation_->IsAutoloadEnabled())) emulation_->ToggleAutoload();
      if (ImGui::MenuItem("Type the clipboard text", nullptr, false, SDL_HasClipboardText()))
      {
         char* text = SDL_GetClipboardText();
         if (text != nullptr)
         {
            emulation_->AutoType(text);
            SDL_free(text);
         }
      }
      ImGui::Separator();
      if (ImGui::MenuItem("Exit")) host_->Quit();
      ImGui::EndMenu();
   }

   if (ImGui::BeginMenu("Control"))
   {
      if (ImGui::MenuItem("Power on / off")) emulation_->HardReset();
      if (ImGui::MenuItem("Pause", nullptr, !emulation_->EmulationRun())) emulation_->Pause();
      if (ImGui::BeginMenu("Speed"))
      {
         static const int speeds[] = { 10, 50, 100, 150, 200, 400 };
         for (const int speed : speeds)
         {
            char label[16];
            std::snprintf(label, sizeof(label), "%d %%", speed);
            if (ImGui::MenuItem(label, nullptr, IsSpeed(SPEED_CUSTOM, speed))) SetSpeed(SPEED_CUSTOM, speed);
         }
         ImGui::Separator();
         if (ImGui::MenuItem("G-Sync / FreeSync (frames paced, sound follows)", nullptr, IsSpeed(SPEED_GSYNC, 0))) SetSpeed(SPEED_GSYNC, 0);
         if (ImGui::MenuItem("Sound", nullptr, IsSpeed(SPEED_SOUND, 0))) SetSpeed(SPEED_SOUND, 0);
         if (ImGui::MenuItem("Screen refresh (VSync)", nullptr, IsSpeed(SPEED_VSYNC, 0))) SetSpeed(SPEED_VSYNC, 0);
         if (ImGui::MenuItem("Maximum", nullptr, IsSpeed(SPEED_FULL, 0))) SetSpeed(SPEED_FULL, 0);
         ImGui::EndMenu();
      }
      ImGui::EndMenu();
   }

   if (ImGui::BeginMenu("Machine"))
   {
      MachineSettings* current = engine->GetSettings();
      for (unsigned int i = 0; i < settings_list_.GetNumberOfConfigurations(); i++)
      {
         MachineSettings* settings = settings_list_.GetConfiguration(i);
         if (settings == nullptr) continue;
         const bool selected = current != nullptr && *current == *settings;
         ImGui::PushID(static_cast<int>(i));
         if (ImGui::MenuItem(settings->GetShortDescription(), nullptr, selected))
         {
            settings->Load();
            emulation_->ChangeConfig(settings);
         }
         ImGui::PopID();
      }
      ImGui::EndMenu();
   }

   const bool fdc_present = engine->GetSettings() == nullptr || engine->GetSettings()->FDCPlugged();
   if (ImGui::BeginMenu("Disk", fdc_present))
   {
      if (ImGui::BeginMenu("Drive A"))
      {
         DrawDiskMenu(0);
         ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("Drive B"))
      {
         DrawDiskMenu(1);
         ImGui::EndMenu();
      }
      ImGui::EndMenu();
   }

   if (ImGui::BeginMenu("Tape"))
   {
      if (ImGui::MenuItem("Insert...")) OpenDialog(DLG_INSERT_TAPE);
      if (ImGui::MenuItem("Save as...")) OpenDialog(DLG_SAVE_TAPE);
      ImGui::Separator();
      if (ImGui::MenuItem("Play")) emulation_->TapePlay();
      if (ImGui::MenuItem("Record")) emulation_->TapeRecord();
      if (ImGui::MenuItem("Pause")) emulation_->TapePause();
      if (ImGui::MenuItem("Stop")) emulation_->TapeStop();
      if (ImGui::MenuItem("Rewind")) emulation_->TapeRewind();
      if (ImGui::MenuItem("Fast forward")) emulation_->TapeFastForward();
      ImGui::EndMenu();
   }

   if (ImGui::BeginMenu("Snapshot"))
   {
      const bool quick_available = engine->IsQuickSnapAvailable();
      if (ImGui::MenuItem("Load...")) OpenDialog(DLG_LOAD_SNA);
      if (ImGui::MenuItem("Save...")) OpenDialog(DLG_SAVE_SNA);
      if (ImGui::MenuItem("Quick load", nullptr, false, quick_available)) emulation_->QuickLoadsnapshot();
      if (ImGui::MenuItem("Quick save", nullptr, false, quick_available)) emulation_->QuickSavesnapshot();
      ImGui::Separator();
      if (ImGui::MenuItem("Play a record (SNR)...")) OpenDialog(DLG_LOAD_SNR);
      if (ImGui::MenuItem("Stop the playback", nullptr, false, engine->IsSnrReplaying()))
      {
         const Emulation::CommandLock lock(emulation_);
         engine->StopPlayback();
      }
      if (ImGui::MenuItem("Record (SNR)...")) OpenDialog(DLG_RECORD_SNR);
      if (ImGui::MenuItem("Stop the record", nullptr, false, engine->IsSnrRecording()))
      {
         const Emulation::CommandLock lock(emulation_);
         engine->StopRecord();
      }
      ImGui::EndMenu();
   }

   if (ImGui::BeginMenu("Cartridge"))
   {
      if (ImGui::MenuItem("Load...")) OpenDialog(DLG_LOAD_CPR);
      ImGui::EndMenu();
   }

   if (ImGui::BeginMenu("Display"))
   {
      if (ImGui::MenuItem("Full screen", "Alt+Enter", host_->IsFullScreen())) host_->ToggleFullScreen();
      ImGui::Separator();
      if (ImGui::MenuItem("Screenshot", "F12")) QuickScreenshot();
      if (ImGui::MenuItem("Save screenshot as...")) OpenDialog(DLG_SAVE_SCREENSHOT);
      ImGui::EndMenu();
   }

   if (ImGui::BeginMenu("Sound"))
   {
      bool muted = sound_->IsMuted();
      if (ImGui::MenuItem("Mute", nullptr, muted)) sound_->Mute(!muted);
      float volume = sound_->GetVolume() * 100.0f;
      ImGui::SetNextItemWidth(ImGui::GetFontSize() * 10.0f);
      if (ImGui::SliderFloat("Volume", &volume, 0.0f, 100.0f, "%.0f %%")) sound_->SetVolume(volume / 100.0f);
      ImGui::EndMenu();
   }

   ImGui::EndMainMenuBar();
}

void UserInterface::DrawDriveStatus(int drive)
{
   EmulatorEngine* engine = emulation_->GetEngine();
   // 0 : no disk; 1 : idle; 2 : read; 3 : write
   const int status = engine->IsDiskRunning(drive);
   static const ImU32 colors[] = {
      IM_COL32(0x70, 0x70, 0x70, 0xFF), IM_COL32(0x90, 0x00, 0x00, 0xFF), IM_COL32(0x00, 0xFF, 0x00, 0xFF), IM_COL32(0xFF, 0x00, 0x00, 0xFF) };

   ImGui::SameLine(0.0f, ImGui::GetFontSize());
   ImGui::BeginGroup();
   ImGui::Text("%c:", 'A' + drive);
   ImGui::SameLine(0.0f, ImGui::GetFontSize() * 0.3f);

   const float size = ImGui::GetFontSize();
   const ImVec2 position = ImGui::GetCursorScreenPos();
   ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2(position.x + size * 0.5f, position.y + size * 0.5f), size * 0.35f,
      colors[(status >= 0 && status <= 3) ? status : 0]);
   ImGui::Dummy(ImVec2(size, size));
   if (status != 0)
   {
      ImGui::SameLine(0.0f, ImGui::GetFontSize() * 0.3f);
      ImGui::Text("%02d", engine->GetFDC()->GetCurrentTrack(drive));
   }
   ImGui::EndGroup();
   if (status != 0 && ImGui::IsItemHovered())
   {
      const char* path = engine->GetFDC()->GetDiskPath(drive);
      if (path != nullptr && path[0] != 0) ImGui::SetTooltip("%s", path);
   }
}

void UserInterface::DrawStatusBar(const Statistics& statistics)
{
   const ImGuiViewport* viewport = ImGui::GetMainViewport();
   const ImGuiStyle& style = ImGui::GetStyle();
   const float height = ImGui::GetFrameHeight();

   ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + viewport->Size.y - height));
   ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, height));
   ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(style.WindowPadding.x, style.FramePadding.y));
   ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(1.0f, 1.0f));
   ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
   ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
   ImGui::PushStyleColor(ImGuiCol_WindowBg, ImGui::GetStyleColorVec4(ImGuiCol_MenuBarBg));
   const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings
      | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus;
   if (ImGui::Begin("##status", nullptr, flags))
   {
      bottom_margin_ = static_cast<int>(height * ImGui::GetIO().DisplayFramebufferScale.y);

      EmulatorEngine* engine = emulation_->GetEngine();
      if (!emulation_->EmulationRun())
      {
         ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Paused");
      }
      else
      {
         ImGui::Text("%u %%", statistics.speed);
      }
      if (ImGui::IsItemHovered())
      {
         ImGui::SetTooltip("%.1f frames/s emulated, %.1f shown", statistics.emulated_frames, statistics.shown_frames);
      }

      if (engine->GetSettings() == nullptr || engine->GetSettings()->FDCPlugged())
      {
         DrawDriveStatus(0);
         DrawDriveStatus(1);
      }

      const unsigned int tape_length = engine->GetTape()->LengthOfTape();
      if (tape_length > 0)
      {
         const unsigned int counter = engine->GetTape()->GetCounter();
         ImGui::SameLine(0.0f, ImGui::GetFontSize());
         ImGui::Text("Tape %02u:%02u / %02u:%02u", counter / 60, counter % 60, tape_length / 60, tape_length % 60);
      }

      if (sound_->IsMuted())
      {
         ImGui::SameLine(0.0f, ImGui::GetFontSize());
         ImGui::TextDisabled("Muted");
      }

      if (!message_.empty())
      {
         if (SDL_GetTicks() < message_end_)
         {
            ImGui::SameLine(0.0f, ImGui::GetFontSize());
            ImGui::TextUnformatted(message_.c_str());
         }
         else
         {
            message_.clear();
         }
      }
   }
   ImGui::End();
   ImGui::PopStyleColor();
   ImGui::PopStyleVar(4);
}

///////////////////////////////////////////////////
// Dialogs

bool UserInterface::Confirm(const char* message)
{
   const SDL_MessageBoxButtonData buttons[] = {
      { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "Cancel" },
      { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "Continue" } };
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

bool UserInterface::ConfirmDiskChange(int drive)
{
   if (!emulation_->GetEngine()->IsDiskModified(drive)) return true;
   return Confirm(drive == 0
      ? "The disk in drive A was modified, and not saved.\nContinue, and lose the changes ?"
      : "The disk in drive B was modified, and not saved.\nContinue, and lose the changes ?");
}

bool UserInterface::ConfirmTapeChange()
{
   if (!emulation_->GetEngine()->IsTapeChanged()) return true;
   return Confirm("The tape was modified, and not saved.\nContinue, and lose the changes ?");
}

void UserInterface::OpenDialog(DialogKind kind, int drive)
{
   // Freed by the callback, which is always called (once)
   auto* request = new DialogRequest{ this, kind, drive };
   switch (kind)
   {
   case DLG_OPEN_MEDIA:
      SDL_ShowOpenFileDialog(DialogCallback, request, window_, media_filters_.data(), static_cast<int>(media_filters_.size()), nullptr, false);
      break;
   case DLG_RUN_SCRIPT:
      SDL_ShowOpenFileDialog(DialogCallback, request, window_, kScriptFilters, 2, nullptr, false);
      break;
   case DLG_INSERT_DISK:
      SDL_ShowOpenFileDialog(DialogCallback, request, window_, disk_load_filters_.data(), static_cast<int>(disk_load_filters_.size()), nullptr, false);
      break;
   case DLG_SAVE_DISK:
      SDL_ShowSaveFileDialog(DialogCallback, request, window_, disk_save_filters_.data(), static_cast<int>(disk_save_filters_.size()), nullptr);
      break;
   case DLG_INSERT_TAPE:
      SDL_ShowOpenFileDialog(DialogCallback, request, window_, kTapeLoadFilters, 2, nullptr, false);
      break;
   case DLG_SAVE_TAPE:
      SDL_ShowSaveFileDialog(DialogCallback, request, window_, kTapeSaveFilters, kNbTapeSaveFormats, nullptr);
      break;
   case DLG_LOAD_SNA:
      SDL_ShowOpenFileDialog(DialogCallback, request, window_, kSnaFilters, 2, nullptr, false);
      break;
   case DLG_SAVE_SNA:
      SDL_ShowSaveFileDialog(DialogCallback, request, window_, kSnaFilters, 1, nullptr);
      break;
   case DLG_LOAD_SNR:
      SDL_ShowOpenFileDialog(DialogCallback, request, window_, kSnrFilters, 2, nullptr, false);
      break;
   case DLG_RECORD_SNR:
      SDL_ShowSaveFileDialog(DialogCallback, request, window_, kSnrFilters, 1, nullptr);
      break;
   case DLG_LOAD_CPR:
      SDL_ShowOpenFileDialog(DialogCallback, request, window_, kCprFilters, 2, nullptr, false);
      break;
   case DLG_SAVE_SCREENSHOT:
      SDL_ShowSaveFileDialog(DialogCallback, request, window_, kPngFilters, 1, nullptr);
      break;
   }
}

// May be called from another thread : the file is only queued for the main thread
void SDLCALL UserInterface::DialogCallback(void* userdata, const char* const* filelist, int filter)
{
   DialogRequest* request = static_cast<DialogRequest*>(userdata);
   if (filelist == nullptr)
   {
      SDL_Log("File dialog error : %s", SDL_GetError());
   }
   else if (filelist[0] != nullptr)
   {
      std::lock_guard<std::mutex> lock(request->ui->results_mutex_);
      request->ui->results_.push_back({ request->kind, request->drive, filelist[0], filter });
   }
   delete request;
}

void UserInterface::HandleDialogResults()
{
   std::vector<DialogResult> results;
   {
      std::lock_guard<std::mutex> lock(results_mutex_);
      results.swap(results_);
   }
   for (const auto& result : results)
   {
      HandleDialogResult(result);
   }
}

void UserInterface::HandleDialogResult(const DialogResult& result)
{
   std::string path = result.path;
   const std::string extension = LowerExtension(path);

   switch (result.kind)
   {
   case DLG_OPEN_MEDIA:
      host_->LoadMedia(path);
      break;

   case DLG_RUN_SCRIPT:
   {
      std::filesystem::path script = std::filesystem::u8path(path);
      emulation_->AddScript(script);
      break;
   }

   case DLG_INSERT_DISK:
      if (ConfirmDiskChange(result.drive)) emulation_->LoadDisk(path.c_str(), result.drive);
      break;

   case DLG_SAVE_DISK:
   {
      // The format : the filter chosen, unless the extension typed is the one of another format
      FormatType* format = nullptr;
      if (result.filter >= 0 && result.filter < static_cast<int>(disk_save_formats_.size()))
      {
         format = disk_save_formats_[result.filter];
      }
      if (!extension.empty() && (format == nullptr || extension != FirstExtension(format->GetFormatExt())))
      {
         for (const auto& candidate : disk_save_formats_)
         {
            if (extension == FirstExtension(candidate->GetFormatExt()))
            {
               format = candidate;
               break;
            }
         }
      }
      if (format == nullptr && !disk_save_formats_.empty()) format = disk_save_formats_[0];
      if (format == nullptr) break;
      if (extension.empty()) path += "." + FirstExtension(format->GetFormatExt());
      emulation_->SaveDiskAs(result.drive, path.c_str(), format);
      break;
   }

   case DLG_INSERT_TAPE:
      if (ConfirmTapeChange()) emulation_->LoadTape(path.c_str());
      break;

   case DLG_SAVE_TAPE:
   {
      int index = result.filter;
      if (index < 0 || index >= kNbTapeSaveFormats || (!extension.empty() && extension != kTapeSaveFilters[index].pattern))
      {
         index = 0;
         for (int i = 0; i < kNbTapeSaveFormats; i++)
         {
            if (extension == kTapeSaveFilters[i].pattern)
            {
               index = i;
               break;
            }
         }
      }
      if (extension.empty()) path += std::string(".") + kTapeSaveFilters[index].pattern;
      emulation_->SaveTapeAs(path.c_str(), kTapeSaveFormats[index]);
      break;
   }

   case DLG_LOAD_SNA:
      emulation_->LoadSnapshot(path.c_str());
      break;

   case DLG_SAVE_SNA:
      if (extension.empty()) path += ".sna";
      emulation_->SaveSnapshot(path.c_str());
      break;

   case DLG_LOAD_SNR:
      emulation_->LoadSnr(path.c_str());
      break;

   case DLG_RECORD_SNR:
      if (extension.empty()) path += ".snr";
      emulation_->RecordSnr(path.c_str());
      break;

   case DLG_LOAD_CPR:
      if (extension == "xpr") emulation_->LoadXpr(path.c_str());
      else emulation_->LoadCpr(path.c_str());
      break;

   case DLG_SAVE_SCREENSHOT:
      if (extension != "png") path += ".png";
      Screenshot(path);
      break;
   }
}

///////////////////////////////////////////////////
// Screenshots

void UserInterface::ShowMessage(const std::string& message)
{
   message_ = message;
   message_end_ = SDL_GetTicks() + 4000;
}

void UserInterface::Screenshot(const std::string& path)
{
   std::vector<unsigned char> rgba;
   int width = 0, height = 0;
   if (display_->CaptureFrameRGBA(rgba, width, height) && ImageWriter::SavePng(path.c_str(), rgba.data(), width, height))
   {
      ShowMessage("Screenshot : " + std::filesystem::u8path(path).filename().u8string());
   }
   else
   {
      ShowMessage("Screenshot failed : " + path);
   }
}

void UserInterface::QuickScreenshot()
{
   std::filesystem::path folder;
   const char* pictures = SDL_GetUserFolder(SDL_FOLDER_PICTURES);
   folder = (pictures != nullptr) ? std::filesystem::u8path(pictures) : std::filesystem::current_path();
   folder /= "Sugarbox";
   std::error_code error;
   std::filesystem::create_directories(folder, error);

   // Sugarbox_YYYYMMDD_HHMMSS(_n).png
   SDL_Time now = 0;
   SDL_DateTime date = {};
   SDL_GetCurrentTime(&now);
   SDL_TimeToDateTime(now, &date, true);
   char name[64];
   std::snprintf(name, sizeof(name), "Sugarbox_%04d%02d%02d_%02d%02d%02d", date.year, date.month, date.day, date.hour, date.minute, date.second);
   std::filesystem::path path = folder / (std::string(name) + ".png");
   for (int index = 2; std::filesystem::exists(path, error); index++)
   {
      path = folder / (std::string(name) + "_" + std::to_string(index) + ".png");
   }
   Screenshot(path.u8string());
}
