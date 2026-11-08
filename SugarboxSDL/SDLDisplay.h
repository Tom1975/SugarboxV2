#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <vector>

#include <SDL3/SDL.h>

#include "Screen.h"

// Display of the emulated screen with SDL_GPU (Direct3D 12, Vulkan or Metal). On Windows the
// swapchain is a DXGI flip model one : a variable refresh rate screen (G-Sync, FreeSync) follows
// the frames in a window as in full screen.
//
// Threads : the emulation thread draws in a frame, and hands it over at each VSync (when the
// frame pacer says it is due). The main thread is woken by an SDL event, and presents it at once.
class SDLDisplay : public IDisplay
{
public:
   SDLDisplay();
   virtual ~SDLDisplay();

   // Main thread
   bool Init(SDL_Window* window);
   void Shutdown();
   // Event pushed when a frame is ready
   Uint32 GetFrameEventType() const { return frame_event_type_; }
   // Presents the last frame handed over (again, if there is no new one)
   void Render();

   // IDisplay
   unsigned int ConvertRGB(unsigned int rgb) override { return 0xFF000000 | rgb; }
   void SetScanlines(int scan) override {}
   bool AFrameIsReady() override { return true; }
   void Display() override {}
   void Config() override {}
   const char* GetInformations() override { return "SDL3 GPU"; }
   int GetWidth() override { return kWidth; }
   int GetHeight() override { return kHeight * 2; }
   void SetSize(SizeEnum size) override {}
   SizeEnum GetSize() override { return S_STANDARD; }
   void VSync(bool dbg = false) override;
   void SyncOnFrame(bool set) override;
   void StartSync() override {}
   void WaitVbl() override {}
   int* GetVideoBuffer(int y) override;
   void Reset() override;
   void FullScreenToggle() override {}
   void ForceFullScreen(bool fullscreen) override {}
   void Screenshot(const char* scr_path) override {}
   void ScreenshotEveryFrame(int on) override {}
   bool IsEveryFrameScreened() override { return false; }
   bool SetSyncWithVbl(int speed) override { return false; }
   bool IsWaitHandled() override { return false; }
   bool IsDisplayed() override { return true; }
   bool GetBlackScreenInterval() override { return false; }
   void SetBlackScreenInterval(bool on) override {}
   void WindowChanged(int x_in, int y_in, int wnd_width, int wnd_height) override {}
   void ResetLoadingMedia() override {}
   void SetLoadingMedia() override {}
   void ResetDragnDropDisplay() override {}
   void SetDragnDropDisplay(int type) override {}
   void SetCurrentPart(int x, int y) override {}
   int GetDnDPart() override { return 0; }
   void SetFrameCallback(std::function<void()> callback) override { frame_callback_ = std::move(callback); }

protected:
   // The emulated monitor draws lines of 1024 pixels : the visible part is cropped from them, as
   // the Qt display does, and each line is shown twice (272 lines -> 544 pixels high)
   static const int kBufferWidth = 1024;
   static const int kBufferHeight = 1024;
   static const int kOriginX = 145;
   static const int kOriginY = 25;
   static const int kWidth = 768;
   static const int kHeight = 272;

   // Main thread : takes the last frame handed over, if any
   bool TakeFrame();

   // Frames : one drawn by the emulation, one ready, one shown. The emulation never waits for the
   // display (unless it is synchronised on it) : a frame not shown in time is replaced by the newer.
   std::vector<int> frames_[3];
   int write_index_ = 0;
   int ready_index_ = 1;
   int read_index_ = 2;
   bool frame_ready_ = false;
   bool sync_on_frame_ = false;
   std::mutex mutex_;
   std::condition_variable frame_taken_;
   std::atomic<bool> event_pending_{ false };
   std::function<void()> frame_callback_;

   // SDL
   SDL_Window* window_ = nullptr;
   SDL_GPUDevice* device_ = nullptr;
   SDL_GPUTexture* texture_ = nullptr;
   SDL_GPUTransferBuffer* transfer_buffer_ = nullptr;
   Uint32 frame_event_type_ = 0;
   bool texture_has_frame_ = false;
};
