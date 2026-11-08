// Sugarbox, SDL3 frontend.
// First step of the move away from Qt : the emulation, its display (SDL_GPU, variable refresh rate
// in a window as in full screen), its sound (SDL audio stream), the keyboard, and the media given
// on the command line or dropped on the window. Menus, settings and debugger come later.

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#ifdef _WIN32
#include <windows.h>
#include <timeapi.h>
#endif

#include "Emulation.h"
#include "Script.h"
#include "ScriptContext.h"

#include "KeyboardMap.h"
#include "SDLDisplay.h"
#include "SDLSound.h"

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

   class SugarboxSDL : public INotifier, public ISoundFactory
   {
   public:
      SugarboxSDL() : emulation_(this) {}

      int Run(int argc, char* argv[]);

      // INotifier
      void DiskLoaded() override {}
      void DiskInserted(int drive) override {}
      void DiskEjected() override {}

      // ISoundFactory
      ISound* GetSound(const char* name) override { return &sound_; }
      const char* GetSoundName(ISound*) override { return "SDL3 audio"; }
      const char* GetFirstSoundName() override { return "SDL3 audio"; }
      const char* GetNextSoundName() override { return nullptr; }

   protected:
      bool ParseCommandLine(int argc, char* argv[]);
      void HandleEvent(const SDL_Event& event);
      void LoadMedia(const std::string& path);
      void ToggleFullScreen();

      SDL_Window* window_ = nullptr;
      SDLDisplay display_;
      SDLSound sound_;
      Emulation emulation_;
      ScriptContext script_context_;
      IKeyboard* keyboard_ = nullptr;

      SugarboxInitialisation init_ = {};
      std::vector<std::string> media_to_load_;
      // Speed limit from the command line (empty : the one of Sugarbox.ini)
      std::string speed_;
      bool running_ = true;
   };

   bool SugarboxSDL::ParseCommandLine(int argc, char* argv[])
   {
      for (int i = 1; i < argc; i++)
      {
         const std::string arg = argv[i];
         auto value = [&]() -> std::string { return (i + 1 < argc) ? argv[++i] : std::string(); };

         if (arg == "-s" || arg == "--csl") init_._script_to_run = value();
         else if (arg == "--cfg" || arg == "--config") init_._hardware_configuration = value();
         else if (arg == "--cart" || arg == "--cartridge") init_._cart_inserted = value();
         else if (arg == "-d" || arg == "--debug") init_._debug_start = true;
         else if (arg == "--speed") speed_ = value();
         else if (arg == "-h" || arg == "--help")
         {
            std::printf("SugarboxSDL [options] [media...]\n"
                        "  -s, --csl <script>    Run a CSL script\n"
                        "  --cfg <configuration> Configuration to load\n"
                        "  --cart <path>         Cartridge to insert\n"
                        "  -d, --debug           Start in break\n"
                        "  --speed <mode>        gsync, vsync, sound, full, or a percentage\n"
                        "  media                 Disk, tape, snapshot... to load\n"
                        "Alt+Enter : full screen\n");
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
      if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
      {
         SDL_Log("SDL_Init failed : %s", SDL_GetError());
         return 1;
      }

      // 768 x 544 (the Qt display), at the scale of the screen
      float scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
      if (scale <= 0.0f) scale = 1.0f;
      window_ = SDL_CreateWindow("Sugarbox", static_cast<int>(768 * scale), static_cast<int>(544 * scale), SDL_WINDOW_RESIZABLE);
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

      // No menu yet : the speed limit can be chosen on the command line
      EmulatorEngine* engine = emulation_.GetEngine();
      if (speed_ == "gsync") engine->SetSpeedLimit(EmulatorEngine::E_SOUND_AND_VBL);
      else if (speed_ == "vsync") engine->SetSpeedLimit(EmulatorEngine::E_VBL);
      else if (speed_ == "sound") engine->SetSpeedLimit(EmulatorEngine::E_SOUND);
      else if (speed_ == "full") engine->SetSpeedLimit(EmulatorEngine::E_FULL);
      else if (!speed_.empty())
      {
         const int percent = std::atoi(speed_.c_str());
         if (percent > 0)
         {
            engine->SetSpeedLimit(EmulatorEngine::E_CUSTOM);
            engine->SetSpeed(percent);
         }
      }

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

      // Main loop : woken by the events, and by each frame handed over by the emulation, which
      // is presented at once
      while (running_)
      {
         SDL_Event event;
         if (SDL_WaitEventTimeout(&event, 100))
         {
            HandleEvent(event);
            while (running_ && SDL_PollEvent(&event))
            {
               HandleEvent(event);
            }
         }
      }

      emulation_.Stop();
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
         display_.Render();
         return;
      }

      switch (event.type)
      {
      case SDL_EVENT_QUIT:
         running_ = false;
         break;

      case SDL_EVENT_WINDOW_EXPOSED:
      case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
         // Shown again : the last frame (the emulation may be paused)
         display_.Render();
         break;

      case SDL_EVENT_KEY_DOWN:
         if (event.key.scancode == SDL_SCANCODE_RETURN && (event.key.mod & SDL_KMOD_ALT) != 0)
         {
            if (!event.key.repeat) ToggleFullScreen();
            break;
         }
         if (!event.key.repeat && keyboard_ != nullptr)
         {
            const unsigned int code = SdlScancodeToHost(event.key.scancode);
            if (code != 0) keyboard_->SendScanCode(code, true);
         }
         break;

      case SDL_EVENT_KEY_UP:
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

   void SugarboxSDL::ToggleFullScreen()
   {
      const bool full_screen = (SDL_GetWindowFlags(window_) & SDL_WINDOW_FULLSCREEN) != 0;
      SDL_SetWindowFullscreen(window_, !full_screen);
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
         // TODO : ask to save the disk in the drive when it was modified (no dialog yet)
         emulation_.LoadDisk(container, 0);
         break;
      case MediaManager::MEDIA_TAPE:
      {
         auto list = container->GetFileList();
         if (!list.empty()) emulation_.LoadTape(list[0]);
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
