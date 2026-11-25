#include "SDLSound.h"

#include <chrono>
#include <thread>

#include "Machine.h"

SDLSound::SDLSound()
{
   for (auto& buffer : buffers_)
   {
      buffer.data_ = nullptr;
      buffer.buffer_length_ = 0;
      buffer.status_ = IWaveHDR::UNUSED;
   }
   Init(44100, 16, 2);
}

SDLSound::~SDLSound()
{
   Close();
   for (auto& buffer : buffers_)
   {
      delete[] buffer.data_;
   }
}

bool SDLSound::Open()
{
   SDL_AudioSpec spec = {};
   spec.format = SDL_AUDIO_S16;
   spec.channels = nb_channels_;
   spec.freq = sample_rate_;

   device_ = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec);
   effects_device_ = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec);
   if (device_ == 0)
   {
      SDL_Log("SDL_OpenAudioDevice failed : %s", SDL_GetError());
      return false;
   }

   stream_ = SDL_CreateAudioStream(&spec, &spec);
   if (stream_ == nullptr || !SDL_BindAudioStream(device_, stream_))
   {
      SDL_Log("Audio stream creation failed : %s", SDL_GetError());
      return false;
   }

   // Started by the first buffer, once the prebuffering is reached
   SDL_PauseAudioDevice(device_);
   playing_ = false;
   return true;
}

void SDLSound::Close()
{
   for (auto& wav : wav_list_)
   {
      SDL_DestroyAudioStream(wav.second.stream);
      SDL_free(wav.second.data);
   }
   wav_list_.clear();

   if (stream_ != nullptr) SDL_DestroyAudioStream(stream_);
   if (device_ != 0) SDL_CloseAudioDevice(device_);
   if (effects_device_ != 0) SDL_CloseAudioDevice(effects_device_);
   stream_ = nullptr;
   device_ = 0;
   effects_device_ = 0;
}

///////////////////////////////////////////////////
// ISound

bool SDLSound::Init(int sample_rate, int sample_bits, int nb_channels)
{
   sample_rate_ = sample_rate;
   sample_bits_ = sample_bits;
   nb_channels_ = nb_channels;

   for (auto& buffer : buffers_)
   {
      delete[] buffer.data_;
      buffer.buffer_length_ = kBufferFrames * BytesPerFrame();
      buffer.data_ = new char[buffer.buffer_length_];
      buffer.status_ = IWaveHDR::UNUSED;
   }
   return true;
}

// Called when the mixer thread (re)starts : the stream starts again empty
void SDLSound::Reinit()
{
   if (stream_ != nullptr)
   {
      SDL_ClearAudioStream(stream_);
      SDL_PauseAudioDevice(device_);
   }
   playing_ = false;
   for (auto& buffer : buffers_)
   {
      buffer.status_ = IWaveHDR::UNUSED;
   }
}

IWaveHDR* SDLSound::GetFreeBuffer()
{
   for (auto& buffer : buffers_)
   {
      if (buffer.status_ == IWaveHDR::UNUSED)
      {
         buffer.status_ = IWaveHDR::USED;
         return &buffer;
      }
   }
   return nullptr;
}

void SDLSound::AddBufferToPlay(IWaveHDR* buffer)
{
   if (stream_ == nullptr)
   {
      buffer->status_ = IWaveHDR::UNUSED;
      return;
   }

   const int queued_before = SDL_GetAudioStreamQueued(stream_);
   // Copied by SDL : the buffer is free again at once
   SDL_PutAudioStreamData(stream_, buffer->data_, buffer->buffer_length_);
   buffer->status_ = IWaveHDR::UNUSED;

   if (playing_ && queued_before == 0 && prebuffer_bytes_ > 0)
   {
      // Ran dry : wait for the whole latency again, rather than running dry at once
      SDL_PauseAudioDevice(device_);
      playing_ = false;
   }
   if (!playing_ && SDL_GetAudioStreamQueued(stream_) >= static_cast<int>(prebuffer_bytes_))
   {
      SDL_ResumeAudioDevice(device_);
      playing_ = true;
   }
}

// Speed limited on the sound : wait until there are only a few buffers ahead
void SDLSound::SyncWithSound()
{
   if (stream_ == nullptr || !sync_on_sound_) return;

   const int max_queued = 2 * kBufferFrames * BytesPerFrame();
   const auto start = std::chrono::steady_clock::now();
   while (sync_on_sound_ && playing_ && SDL_GetAudioStreamQueued(stream_) > max_queued)
   {
      // A stalled device must not stop the emulation (it waits with the machine locked)
      if (std::chrono::steady_clock::now() - start > std::chrono::milliseconds(100))
         break;
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
   }
}

int SDLSound::GetPendingSamples()
{
   if (stream_ == nullptr) return -1;
   const int queued = SDL_GetAudioStreamQueued(stream_);
   return queued < 0 ? -1 : queued / BytesPerFrame();
}

void SDLSound::SetPrebuffer(unsigned int nb_samples)
{
   prebuffer_bytes_ = nb_samples * BytesPerFrame();
}

///////////////////////////////////////////////////
// Drive sounds

void SDLSound::AddWav(int id, const unsigned char* buffer, unsigned int size)
{
   if (effects_device_ == 0) return;

   // The size given is not always the one of this file : the RIFF header has the right one
   if (size >= 8 && buffer[0] == 'R' && buffer[1] == 'I' && buffer[2] == 'F' && buffer[3] == 'F')
   {
      size = 8 + (buffer[4] | (buffer[5] << 8) | (buffer[6] << 16) | (static_cast<unsigned int>(buffer[7]) << 24));
   }

   Wav wav;
   SDL_AudioSpec spec = {};
   SDL_IOStream* io = SDL_IOFromConstMem(buffer, size);
   if (io == nullptr || !SDL_LoadWAV_IO(io, true, &spec, &wav.data, &wav.length))
   {
      SDL_Log("Drive sound %i not loaded : %s", id, SDL_GetError());
      return;
   }

   wav.stream = SDL_CreateAudioStream(&spec, nullptr);
   if (wav.stream == nullptr || !SDL_BindAudioStream(effects_device_, wav.stream))
   {
      SDL_free(wav.data);
      return;
   }
   wav_list_[id] = wav;
}

void SDLSound::PlayWav(int wav_registered)
{
   auto wav = wav_list_.find(wav_registered);
   if (wav == wav_list_.end()) return;

   SDL_ClearAudioStream(wav->second.stream);
   SDL_PutAudioStreamData(wav->second.stream, wav->second.data, wav->second.length);
}

///////////////////////////////////////////////////
// ISoundNotification

void SDLSound::ApplyGain()
{
   if (stream_ != nullptr)
   {
      SDL_SetAudioStreamGain(stream_, mute_ ? 0.0f : volume_);
   }
}

void SDLSound::Mute(bool mute)
{
   mute_ = mute;
   ApplyGain();
}

void SDLSound::SetVolume(float volume)
{
   volume_ = volume;
   if (volume > 0) mute_ = false;
   ApplyGain();
}

void SDLSound::Record(bool on)
{
   if (emulation_ == nullptr) return;

   if (emulation_->IsRecording())
   {
      if (!on) emulation_->EndRecord();
   }
   else if (on)
   {
      emulation_->BeginRecord();
   }
}

bool SDLSound::IsRecordOn()
{
   return emulation_ != nullptr && emulation_->IsRecording();
}
