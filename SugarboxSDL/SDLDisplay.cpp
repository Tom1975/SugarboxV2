#include "SDLDisplay.h"

#include <chrono>
#include <cstring>

SDLDisplay::SDLDisplay()
{
   for (auto& frame : frames_)
   {
      frame.assign(kBufferWidth * kBufferHeight, 0xFF000000);
   }
}

SDLDisplay::~SDLDisplay()
{
   Shutdown();
}

bool SDLDisplay::Init(SDL_Window* window)
{
   window_ = window;
   frame_event_type_ = SDL_RegisterEvents(1);

   // No shader yet (the frame is copied with a blit) : any format the backends take
   device_ = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL, false, nullptr);
   if (device_ == nullptr)
   {
      SDL_Log("SDL_CreateGPUDevice failed : %s", SDL_GetError());
      return false;
   }
   SDL_Log("GPU driver : %s", SDL_GetGPUDeviceDriver(device_));

   if (!SDL_ClaimWindowForGPUDevice(device_, window_))
   {
      SDL_Log("SDL_ClaimWindowForGPUDevice failed : %s", SDL_GetError());
      return false;
   }

   // VSync present mode : with a variable refresh rate screen, the screen refreshes when the frame
   // comes (within its range), and never tears.
   SDL_SetGPUSwapchainParameters(device_, window_, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC);
   // A single frame in flight : the frame is shown as soon as possible after its hand over
   SDL_SetGPUAllowedFramesInFlight(device_, 1);

   SDL_GPUTextureCreateInfo texture_info = {};
   texture_info.type = SDL_GPU_TEXTURETYPE_2D;
   texture_info.format = SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM;
   texture_info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
   texture_info.width = kWidth;
   texture_info.height = kHeight;
   texture_info.layer_count_or_depth = 1;
   texture_info.num_levels = 1;
   texture_ = SDL_CreateGPUTexture(device_, &texture_info);

   SDL_GPUTransferBufferCreateInfo transfer_info = {};
   transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
   transfer_info.size = kWidth * kHeight * 4;
   transfer_buffer_ = SDL_CreateGPUTransferBuffer(device_, &transfer_info);

   if (texture_ == nullptr || transfer_buffer_ == nullptr)
   {
      SDL_Log("GPU resources creation failed : %s", SDL_GetError());
      return false;
   }
   return true;
}

void SDLDisplay::Shutdown()
{
   if (device_ == nullptr) return;

   SDL_WaitForGPUIdle(device_);
   if (transfer_buffer_ != nullptr) SDL_ReleaseGPUTransferBuffer(device_, transfer_buffer_);
   if (texture_ != nullptr) SDL_ReleaseGPUTexture(device_, texture_);
   SDL_ReleaseWindowFromGPUDevice(device_, window_);
   SDL_DestroyGPUDevice(device_);
   transfer_buffer_ = nullptr;
   texture_ = nullptr;
   device_ = nullptr;
}

///////////////////////////////////////////////////
// Emulation thread

int* SDLDisplay::GetVideoBuffer(int y)
{
   return &frames_[write_index_][kBufferWidth * y];
}

void SDLDisplay::Reset()
{
   std::fill(frames_[write_index_].begin(), frames_[write_index_].end(), 0xFF000000);
}

void SDLDisplay::SyncOnFrame(bool set)
{
   std::lock_guard<std::mutex> lock(mutex_);
   sync_on_frame_ = set;
   frame_taken_.notify_all();
}

void SDLDisplay::VSync(bool dbg)
{
   {
      std::unique_lock<std::mutex> lock(mutex_);
      // Synchronised on the screen refresh : wait until the previous frame is shown. Never for
      // long : a hidden or minimised window must not stop the emulation.
      if (sync_on_frame_ && frame_ready_)
      {
         frame_taken_.wait_for(lock, std::chrono::milliseconds(100), [this]() { return !frame_ready_ || !sync_on_frame_; });
      }
      // The ready frame not shown yet (if any) is replaced by the newer one
      std::swap(write_index_, ready_index_);
      frame_ready_ = true;
   }

   if (frame_callback_) frame_callback_();

   // Wake the main thread up (once, until it handles it)
   if (!event_pending_.exchange(true))
   {
      SDL_Event event = {};
      event.type = frame_event_type_;
      SDL_PushEvent(&event);
   }
}

///////////////////////////////////////////////////
// Main thread

bool SDLDisplay::TakeFrame()
{
   event_pending_ = false;
   std::lock_guard<std::mutex> lock(mutex_);
   if (!frame_ready_) return false;
   std::swap(read_index_, ready_index_);
   frame_ready_ = false;
   frame_taken_.notify_all();
   return true;
}

void SDLDisplay::Render()
{
   if (device_ == nullptr) return;

   const bool new_frame = TakeFrame();

   SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(device_);
   if (command_buffer == nullptr) return;

   if (new_frame)
   {
      // Visible part of the frame -> texture
      Uint8* dest = static_cast<Uint8*>(SDL_MapGPUTransferBuffer(device_, transfer_buffer_, true));
      if (dest != nullptr)
      {
         const int* source = frames_[read_index_].data();
         for (int y = 0; y < kHeight; y++)
         {
            memcpy(dest + y * kWidth * 4, source + (kOriginY + y) * kBufferWidth + kOriginX, kWidth * 4);
         }
         SDL_UnmapGPUTransferBuffer(device_, transfer_buffer_);

         SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer);
         SDL_GPUTextureTransferInfo transfer = {};
         transfer.transfer_buffer = transfer_buffer_;
         transfer.pixels_per_row = kWidth;
         transfer.rows_per_layer = kHeight;
         SDL_GPUTextureRegion region = {};
         region.texture = texture_;
         region.w = kWidth;
         region.h = kHeight;
         region.d = 1;
         SDL_UploadToGPUTexture(copy_pass, &transfer, &region, true);
         SDL_EndGPUCopyPass(copy_pass);
         texture_has_frame_ = true;
      }
   }

   SDL_GPUTexture* swapchain = nullptr;
   Uint32 swapchain_width = 0, swapchain_height = 0;
   if (SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, window_, &swapchain, &swapchain_width, &swapchain_height)
      && swapchain != nullptr)
   {
      // Black borders
      SDL_GPUColorTargetInfo target = {};
      target.texture = swapchain;
      target.clear_color = { 0.0f, 0.0f, 0.0f, 1.0f };
      target.load_op = SDL_GPU_LOADOP_CLEAR;
      target.store_op = SDL_GPU_STOREOP_STORE;
      SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(command_buffer, &target, 1, nullptr);
      SDL_EndGPURenderPass(render_pass);

      if (texture_has_frame_)
      {
         // The largest 768 x 544 rectangle (the shape of the Qt display) that fits, centred
         const float scale = SDL_min(swapchain_width / static_cast<float>(kWidth),
                                     swapchain_height / static_cast<float>(kHeight * 2));
         const Uint32 width = static_cast<Uint32>(kWidth * scale);
         const Uint32 height = static_cast<Uint32>(kHeight * 2 * scale);

         SDL_GPUBlitInfo blit = {};
         blit.source.texture = texture_;
         blit.source.w = kWidth;
         blit.source.h = kHeight;
         blit.destination.texture = swapchain;
         blit.destination.x = (swapchain_width - width) / 2;
         blit.destination.y = (swapchain_height - height) / 2;
         blit.destination.w = width;
         blit.destination.h = height;
         blit.load_op = SDL_GPU_LOADOP_LOAD;
         blit.filter = SDL_GPU_FILTER_NEAREST;
         SDL_BlitGPUTexture(command_buffer, &blit);
      }
   }
   SDL_SubmitGPUCommandBuffer(command_buffer);
}
