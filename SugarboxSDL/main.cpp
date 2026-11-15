// Sugarbox, SDL3 frontend.
// First step of the move away from Qt : the emulation, its display (SDL_GPU, variable refresh rate
// in a window as in full screen), its sound (SDL audio stream), the keyboard, and the media given
// on the command line or dropped on the window, the menus and the status bar (Dear ImGui), the
// screenshots, and the debug server (VS Code debug adapter). Settings dialogs and debugger windows
// come later.

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#ifdef _WIN32
#include <windows.h>
#include <timeapi.h>
#endif

#include "ConfigurationManager.h"
#include "Emulation.h"
#include "SettingsList.h"
#include "Script.h"
#include "ScriptContext.h"
#include "debugers/DebugServer.h"

#include "AppSettings.h"
#include "GamepadInput.h"
#include "KeyboardMap.h"
#include "SDLDisplay.h"
#include "SDLSound.h"
#include "UserInterface.h"

namespace
{
#ifdef _WIN32
   // Since Windows 10 2004, a sleep is only as precise as the timer resolution this process asks
   // for (15.6 ms by default) : the frame and sound pacing needs 1 ms.
   class TimerResolution
   {
   public:
      TimerResolution() { timeBeginPeriod(1); }
      ~TimerResolution() { timeEndPeriod(1); }
   };
#endif

   class SugarboxSDL : public INotifier, public ISoundFactory, public IUserInterfaceHost, public IDebugerStopped
   {
   public:
      SugarboxSDL() : emulation_(this) {}

      int Run(int argc, char* argv[]);

      // INotifier
      void DiskLoaded() override {}
      void DiskInserted(int drive) override { if (debug_server_ != nullptr) debug_server_->NotifyMediaChanged(drive, true); }
      void DiskEjected() override { if (debug_server_ != nullptr) debug_server_->NotifyMediaChanged(-1, false); }

      // IDebugerStopped
      void NotifyStop(Reason reason) override
      {
         if (debug_server_ != nullptr) debug_server_->NotifyStop(reason);
         // Without a debug client, the debugger window shows where it stopped
         ui_.NotifyStop(debug_server_ == nullptr && !init_._no_show);
      }

      // ISoundFactory
      ISound* GetSound(const char* name) override { return &sound_; }
      const char* GetSoundName(ISound*) override { return "SDL3 audio"; }
      const char* GetFirstSoundName() override { return "SDL3 audio"; }
      const char* GetNextSoundName() override { return nullptr; }

      // IUserInterfaceHost
      void LoadMedia(const std::string& path) override;
      void ToggleFullScreen() override;
      bool IsFullScreen() override;
      void Quit() override { running_ = false; }

      // ISettingsHost
      AppSettings& GetSettings() override { return settings_; }
      void ApplySettings() override;
      SettingsList* GetMachines() override { return machines_.get(); }
      void ReloadMachines() override { LoadMachines(); }
      bool IsCurrentMachine(const MachineSettings& machine) override;
      void ChangeMachine(const std::string& path) override;
      std::vector<std::string> GetGamepadNames() override { return gamepads_.GetNames(); }

   protected:
      bool ParseCommandLine(int argc, char* argv[]);
      void HandleEvent(const SDL_Event& event);
      void UpdateStatistics();
      void RenderFrame();
      void StartDebugServer();
      void LoadMachines();
      void SaveSettingsAtExit();

      SDL_Window* window_ = nullptr;
      SDLDisplay display_;
      SDLSound sound_;
      Emulation emulation_;
      ScriptContext script_context_;
      IKeyboard* keyboard_ = nullptr;
      UserInterface ui_;
      GamepadInput gamepads_;
      UserInterface::Statistics statistics_;
      DebugServer* debug_server_ = nullptr;

      // Preferences (Sugarbox.ini)
      AppSettings settings_;
      // Machines of the CONF directory, read with their own configuration manager (the one of the
      // emulation is used by its thread). A list replaced stays alive : the emulation may use
      // one of its machines.
      ConfigurationManager machines_configuration_;
      std::unique_ptr<SettingsList> machines_;
      std::vector<std::unique_ptr<SettingsList>> old_machines_;

      SugarboxInitialisation init_ = {};
      std::vector<std::string> media_to_load_;
      // Speed limit from the command line (empty : the one of the preferences)
      std::string speed_;
      bool running_ = true;
      // A frame of the emulation waits to be shown
      bool frame_due_ = false;
      Uint64 last_render_ = 0;
      Uint64 last_statistics_ = 0;
   };

   bool SugarboxSDL::ParseCommandLine(int argc, char* argv[])
   {
      // The application has no console on Windows : the help and the errors go in a message box
      static const char* usage =
         "SugarboxSDL [options] [media...]\n"
         "  -s, --csl <script>    Run a CSL script\n"
         "  --cfg <configuration> Configuration to load\n"
         "  --cart <path>         Cartridge to insert\n"
         "  -d, --debug           Start in break\n"
         "  --hide                No window shown (tests, with the debug server)\n"
         "  --ds, --debug_server <port>  Debug server (VS Code debug adapter) on this port\n"
         "  --speed <mode>        gsync, vsync, sound, full, or a percentage\n"
         "                        (default : the one of the settings, G-Sync if not set)\n"
         "  media                 Disk, tape, snapshot... to load\n"
         "Options can be written --option value or --option=value.\n"
         "Alt+Enter : full screen (the menu comes with the mouse at the top of the screen)\n"
         "F12 : screenshot (in Pictures/Sugarbox)";

      for (int i = 1; i < argc; i++)
      {
         std::string arg = argv[i];
         // --option=value
         std::string inline_value;
         bool has_inline_value = false;
         const size_t equal = arg.find('=');
         if (arg.rfind("--", 0) == 0 && equal != std::string::npos)
         {
            inline_value = arg.substr(equal + 1);
            arg = arg.substr(0, equal);
            has_inline_value = true;
         }
         auto value = [&]() -> std::string {
            if (has_inline_value) return inline_value;
            return (i + 1 < argc) ? argv[++i] : std::string();
         };

         if (arg == "-s" || arg == "--csl") init_._script_to_run = value();
         else if (arg == "--cfg" || arg == "--config") init_._hardware_configuration = value();
         else if (arg == "--cart" || arg == "--cartridge") init_._cart_inserted = value();
         else if (arg == "-d" || arg == "--debug") init_._debug_start = true;
         else if (arg == "--hide" || arg == "-hide") init_._no_show = true;
         else if (arg == "--ds" || arg == "-ds" || arg == "--debug_server")
         {
            init_._dbs_server = value();
            const unsigned long port = std::strtoul(init_._dbs_server.c_str(), nullptr, 0);
            if (port == 0 || port > 65535)
            {
               const std::string message = "Invalid debug server port : '" + init_._dbs_server + "'\n\n" + usage;
               SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Sugarbox", message.c_str(), nullptr);
               return false;
            }
         }
         else if (arg == "--speed")
         {
            speed_ = value();
            const bool known = speed_ == "gsync" || speed_ == "vsync" || speed_ == "sound" || speed_ == "full";
            if (!known && std::atoi(speed_.c_str()) <= 0)
            {
               const std::string message = "Unknown speed : '" + speed_ + "'\n\n" + usage;
               SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Sugarbox", message.c_str(), nullptr);
               return false;
            }
         }
         else if (arg == "-h" || arg == "--help")
         {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Sugarbox", usage, nullptr);
            return false;
         }
         else if (arg.size() > 1 && arg[0] == '-')
         {
            // Not taken for a media to load : a mistyped option would be silently ignored
            const std::string message = "Unknown option : '" + std::string(argv[i]) + "'\n\n" + usage;
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Sugarbox", message.c_str(), nullptr);
            return false;
         }
         else media_to_load_.push_back(arg);
      }
      return true;
   }

   int SugarboxSDL::Run(int argc, char* argv[])
   {
      if (!ParseCommandLine(argc, argv)) return 0;

      SDL_SetAppMetadata("Sugarbox", "2.1.1", "org.sugarbox.emulator");
      // Read before the emulation : the window and the speed come from them
      settings_.Load("Sugarbox.ini");
      if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD))
      {
         SDL_Log("SDL_Init failed : %s", SDL_GetError());
         return 1;
      }

      // 768 x 544 (the Qt display), at the scale of the screen
      float scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
      if (scale <= 0.0f) scale = 1.0f;
      const SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE | (init_._no_show ? SDL_WINDOW_HIDDEN : 0);
      const bool window_known = settings_.window_width > 0 && settings_.window_height > 0;
      window_ = SDL_CreateWindow("Sugarbox",
         window_known ? settings_.window_width : static_cast<int>(768 * scale),
         window_known ? settings_.window_height : static_cast<int>(544 * scale), window_flags);
      if (window_ != nullptr && window_known)
      {
         SDL_SetWindowPosition(window_, settings_.window_x, settings_.window_y);
      }
      if (window_ != nullptr && settings_.start_full_screen && !init_._no_show)
      {
         SDL_SetWindowFullscreen(window_, true);
      }
      if (window_ == nullptr || !display_.Init(window_))
      {
         SDL_Log("Display creation failed : %s", SDL_GetError());
         SDL_Quit();
         return 1;
      }
      sound_.Open();

      // The emulation reads its files (ROM, CONF, Keyboards, Sugarbox.ini) from the current directory
      const std::string current_path = std::filesystem::current_path().string();
      emulation_.Init(&display_, this, &sound_, current_path.c_str(), init_);
      sound_.SetEmulation(emulation_.GetEngine());
      keyboard_ = emulation_.GetKeyboardHandler();
      gamepads_.Init(keyboard_);
      emulation_.AddNotifierDbg(this);
      StartDebugServer();

      // Speed limit : the one of the command line, else the one of the preferences (G-Sync when
      // Sugarbox.ini has none). The menu changes it for the session.
      EmulatorEngine* engine = emulation_.GetEngine();
      std::string speed = speed_;
      if (speed.empty())
      {
         switch (settings_.limit_speed)
         {
         case 'F': speed = "full"; break;
         case 'V': speed = "vsync"; break;
         case 'S': speed = "sound"; break;
         case 'C': speed = std::to_string(settings_.speed > 0 ? settings_.speed : 100); break;
         default: speed = "gsync"; break;
         }
      }
      if (speed == "gsync") engine->SetSpeedLimit(EmulatorEngine::E_SOUND_AND_VBL);
      else if (speed == "vsync") engine->SetSpeedLimit(EmulatorEngine::E_VBL);
      else if (speed == "sound") engine->SetSpeedLimit(EmulatorEngine::E_SOUND);
      else if (speed == "full") engine->SetSpeedLimit(EmulatorEngine::E_FULL);
      else
      {
         engine->SetSpeedLimit(EmulatorEngine::E_CUSTOM);
         engine->SetSpeed(std::atoi(speed.c_str()));
      }

      LoadMachines();
      // Sound, autoload... (saved once more : the keys missing in Sugarbox.ini are written)
      ApplySettings();

      script_context_.Init(&emulation_, &display_);
      ScriptCommandFactory::InitFactory(&script_context_);
      if (!init_._script_to_run.empty())
      {
         emulation_.AddScript(init_._script_to_run);
      }
      for (const auto& media : media_to_load_)
      {
         LoadMedia(media);
      }

      if (ui_.Init(window_, &display_, &emulation_, &sound_, this, scale))
      {
         // Started in break : the debugger is shown
         if (init_._debug_start && debug_server_ == nullptr) ui_.OpenDebugger();
         // Once the height of the bars is known (first frame) : room for them around the screen
         RenderFrame();
         if (!window_known)
         {
            SDL_SetWindowSize(window_, static_cast<int>(768 * scale),
               static_cast<int>(544 * scale) + ui_.GetTopMargin() + ui_.GetBottomMargin());
         }
      }
      else
      {
         SDL_Log("User interface creation failed : %s", SDL_GetError());
      }

      // Main loop : woken by the events, and by each frame handed over by the emulation.
      // The frames of the emulation set the pace of the screen (variable refresh rate) : the
      // interface is drawn with them, never between two of them. Without them (pause, break),
      // it is drawn 25 times per second.
      const Uint64 idle_period = 40;
      while (running_)
      {
         SDL_Event event;
         if (SDL_WaitEventTimeout(&event, static_cast<Sint32>(idle_period)))
         {
            HandleEvent(event);
            while (running_ && SDL_PollEvent(&event))
            {
               HandleEvent(event);
            }
         }
         ui_.HandleDialogResults();
         UpdateStatistics();
         if (frame_due_ || SDL_GetTicks() - last_render_ >= idle_period)
         {
            RenderFrame();
         }
      }

      SaveSettingsAtExit();

      // The debug server first : its commands wait for the emulation loop
      if (debug_server_ != nullptr)
      {
         debug_server_->stop();
      }
      emulation_.Stop();
      emulation_.RemoveNotifierDbg(this);
      display_.SetFrameCallback(nullptr);
      delete debug_server_;
      debug_server_ = nullptr;
      SDL_WaitForGPUIdle(display_.GetDevice());
      gamepads_.Shutdown();
      ui_.Shutdown();
      display_.Shutdown();
      sound_.Close();
      SDL_DestroyWindow(window_);
      SDL_Quit();
      return 0;
   }

   void SugarboxSDL::HandleEvent(const SDL_Event& event)
   {
      if (event.type == display_.GetFrameEventType())
      {
         frame_due_ = true;
         return;
      }
      ui_.ProcessEvent(event);
      gamepads_.HandleEvent(event);

      switch (event.type)
      {
      case SDL_EVENT_QUIT:
         running_ = false;
         break;

      case SDL_EVENT_WINDOW_EXPOSED:
      case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
         // Shown again : the last frame (the emulation may be paused)
         frame_due_ = true;
         break;

      case SDL_EVENT_KEY_DOWN:
         if (event.key.scancode == SDL_SCANCODE_RETURN && (event.key.mod & SDL_KMOD_ALT) != 0)
         {
            if (!event.key.repeat) ToggleFullScreen();
            break;
         }
         // F12 is not a key of the CPC keyboard maps
         if (event.key.scancode == SDL_SCANCODE_F12)
         {
            if (!event.key.repeat) ui_.QuickScreenshot();
            break;
         }
         if (!event.key.repeat && ui_.CaptureKey(event.key.scancode)) break;
         if (!event.key.repeat && !ui_.WantsKeyboard() && gamepads_.HandleKey(event.key.scancode, true)) break;
         if (!event.key.repeat && keyboard_ != nullptr && !ui_.WantsKeyboard())
         {
            const unsigned int code = SdlScancodeToHost(event.key.scancode);
            if (code != 0) keyboard_->SendScanCode(code, true);
         }
         break;

      case SDL_EVENT_KEY_UP:
         if (gamepads_.HandleKey(event.key.scancode, false)) break;
         if (keyboard_ != nullptr)
         {
            const unsigned int code = SdlScancodeToHost(event.key.scancode);
            if (code != 0) keyboard_->SendScanCode(code, false);
         }
         break;

      case SDL_EVENT_DROP_FILE:
         if (event.drop.data != nullptr)
         {
            LoadMedia(event.drop.data);
         }
         break;

      default:
         break;
      }
   }

   // Once a second : the speed and the frame rates, for the status bar
   void SugarboxSDL::UpdateStatistics()
   {
      const Uint64 now = SDL_GetTicks();
      if (now - last_statistics_ < 1000) return;

      const double seconds = (now - last_statistics_) / 1000.0;
      last_statistics_ = now;
      statistics_.speed = emulation_.GetSpeed();
      statistics_.emulated_frames = display_.TakeEmulatedFrames() / seconds;
      statistics_.shown_frames = display_.TakePresentedFrames() / seconds;
   }

   void SugarboxSDL::RenderFrame()
   {
      frame_due_ = false;
      last_render_ = SDL_GetTicks();
      if (!ui_.IsReady())
      {
         display_.Render();
         return;
      }
      ui_.BuildFrame(statistics_);
      display_.SetMargins(ui_.GetTopMargin(), ui_.GetBottomMargin());
      display_.Render(&ui_);
   }

   void SugarboxSDL::LoadMachines()
   {
      if (machines_) old_machines_.push_back(std::move(machines_));
      machines_ = std::make_unique<SettingsList>();
      const std::filesystem::path conf_path = std::filesystem::current_path() / "CONF";
      machines_->InitSettingsList(&machines_configuration_, conf_path.string().c_str());
      machines_->BuildList();
   }

   bool SugarboxSDL::IsCurrentMachine(const MachineSettings& machine)
   {
      // As the core compares them : by file name
      MachineSettings* current = emulation_.GetEngine()->GetSettings();
      return current != nullptr && *current == const_cast<MachineSettings&>(machine);
   }

   void SugarboxSDL::ChangeMachine(const std::string& path)
   {
      for (unsigned int i = 0; i < machines_->GetNumberOfConfigurations(); i++)
      {
         MachineSettings* machine = machines_->GetConfiguration(i);
         if (machine != nullptr && std::filesystem::path(machine->GetFilePath()) == std::filesystem::path(path))
         {
            machine->Load(true);
            emulation_.ChangeConfig(machine);
            return;
         }
      }
   }

   // The preferences changed : what the running emulator uses now, then Sugarbox.ini
   void SugarboxSDL::ApplySettings()
   {
      display_.SetScaling(static_cast<SDLDisplay::Scaling>(settings_.scaling));
      display_.SetSmooth(settings_.smooth);
      display_.SetScanlines(settings_.scanlines, settings_.scanline_brightness);
      gamepads_.Configure(settings_);
      sound_.SetVolume(settings_.volume);
      sound_.Mute(settings_.mute);
      if (emulation_.IsAutoloadEnabled() != settings_.autoload)
      {
         emulation_.ToggleAutoload();
      }
      // Hidden (tests) : the preferences are read, never written
      if (!init_._no_show) settings_.Save("Sugarbox.ini");
   }

   // What changes while running : the window, the disks in the drives
   void SugarboxSDL::SaveSettingsAtExit()
   {
      if (init_._no_show) return;
      if (!IsFullScreen())
      {
         SDL_GetWindowPosition(window_, &settings_.window_x, &settings_.window_y);
         SDL_GetWindowSize(window_, &settings_.window_width, &settings_.window_height);
      }
      FDC* fdc = emulation_.GetEngine()->GetFDC();
      for (int drive = 0; drive < 2; drive++)
      {
         const char* path = emulation_.IsDiskPresent(drive) ? fdc->GetDiskPath(drive) : nullptr;
         settings_.disks[drive] = (path != nullptr) ? path : "";
      }
      settings_.Save("Sugarbox.ini");
   }

   void SugarboxSDL::StartDebugServer()
   {
      if (init_._dbs_server.empty()) return;

      const unsigned short port = static_cast<unsigned short>(std::strtoul(init_._dbs_server.c_str(), nullptr, 0));
      debug_server_ = new DebugServer(&emulation_, port);
      debug_server_->StartServer();
      // A frame event per VSync, for the clients that subscribed to the screen
      display_.SetFrameCallback([this]() { debug_server_->NotifyFrame(); });
      SDL_Log("Debug server on port %u", port);
   }

   bool SugarboxSDL::IsFullScreen()
   {
      return (SDL_GetWindowFlags(window_) & SDL_WINDOW_FULLSCREEN) != 0;
   }

   void SugarboxSDL::ToggleFullScreen()
   {
      SDL_SetWindowFullscreen(window_, !IsFullScreen());
   }

   // Same handling as the drop on the Qt window
   void SugarboxSDL::LoadMedia(const std::string& path)
   {
      DataContainer* container = emulation_.CanLoad(path.c_str());
      if (container == nullptr)
      {
         std::filesystem::path file_path = path;
         std::string extension = file_path.extension().string();
         for (auto& c : extension) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
         if (extension == ".csl")
         {
            emulation_.AddScript(file_path);
         }
         else
         {
            SDL_Log("Unknown media : %s", path.c_str());
         }
         return;
      }

      MediaManager media_manager(container);
      const std::vector<MediaManager::MediaType> list_of_types = {
         MediaManager::MEDIA_DISK, MediaManager::MEDIA_SNA, MediaManager::MEDIA_SNR, MediaManager::MEDIA_TAPE,
         MediaManager::MEDIA_BIN, MediaManager::MEDIA_CPR, MediaManager::MEDIA_XPR };

      switch (media_manager.GetType(list_of_types))
      {
      case MediaManager::MEDIA_SNA:
         emulation_.LoadSnapshot(path.c_str());
         break;
      case MediaManager::MEDIA_DISK:
         if (ui_.ConfirmDiskChange(0)) emulation_.LoadDisk(container, 0);
         break;
      case MediaManager::MEDIA_TAPE:
      {
         auto list = container->GetFileList();
         if (!list.empty() && ui_.ConfirmTapeChange()) emulation_.LoadTape(list[0]);
         break;
      }
      case MediaManager::MEDIA_SNR:
         emulation_.LoadSnr(path.c_str());
         break;
      case MediaManager::MEDIA_BIN:
         emulation_.LoadBin(path.c_str());
         break;
      case MediaManager::MEDIA_CPR:
         emulation_.LoadCpr(path.c_str());
         break;
      case MediaManager::MEDIA_XPR:
         emulation_.LoadXpr(path.c_str());
         break;
      default:
         SDL_Log("Unsupported media : %s", path.c_str());
         break;
      }
   }
}

int main(int argc, char* argv[])
{
#ifdef _WIN32
   TimerResolution timer_resolution;
#endif
   // Large members (frames, emulation) : not on the stack
   auto* app = new SugarboxSDL();
   const int result = app->Run(argc, argv);
   delete app;
   return result;
}
