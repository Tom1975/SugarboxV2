#pragma once

#include <map>
#include <vector>

#include <SDL3/SDL.h>

#include "IFrontendSound.h"

// Sound output with SDL3 : an audio stream on the default device. SDL converts and mixes the
// streams. The emulated machine's stream and the drive sounds are on two logical devices, so the
// first can wait for its prebuffering without silencing the others.
class SDLSound : public IFrontendSound
{
public:
   SDLSound();
   virtual ~SDLSound();

   // After SDL_Init(SDL_INIT_AUDIO)
   bool Open();
   void Close();

   // IFrontendSound
   void SetEmulation(EmulatorEngine* emulation) override { emulation_ = emulation; }
   void AddWav(int id, const unsigned char* buffer, unsigned int size) override;
   void PlayWav(int wav_registered) override;

   // ICfg
   void SetDefaultConfiguration() override {}
   void SaveConfiguration(const char* config_name, const char* ini_file) override {}
   bool LoadConfiguration(const char* config_name, const char* ini_file) override { return true; }

   // ISound
   bool Init(int sample_rate, int sample_bits, int nb_channels) override;
   void Reinit() override;
   unsigned int GetMaxValue() override { return (1 << sample_bits_) - 1; }
   unsigned int GetMinValue() override { return 0; }
   unsigned int GetSampleRate() override { return sample_rate_; }
   unsigned int GetBitDepth() override { return sample_bits_; }
   unsigned int GetNbChannels() override { return nb_channels_; }
   void CheckBuffersStatus() override {}
   IWaveHDR* GetFreeBuffer() override;
   void AddBufferToPlay(IWaveHDR* buffer) override;
   void SyncOnSound(bool set) override { sync_on_sound_ = set; }
   void SyncWithSound() override;
   int GetPendingSamples() override;
   void SetPrebuffer(unsigned int nb_samples) override;

   // ISoundNotification
   void Mute(bool mute) override;
   bool IsMuted() override { return mute_; }
   void SetVolume(float volume) override;
   float GetVolume() override { return volume_; }
   void Record(bool on) override;
   bool IsRecordOn() override;

protected:
   int BytesPerFrame() const { return nb_channels_ * (sample_bits_ / 8); }
   void ApplyGain();

   // The emulated machine fills buffers of 20 ms, copied at once in the stream
   static const int kNbBuffers = 8;
   static const int kBufferFrames = 882;
   IWaveHDR buffers_[kNbBuffers];

   unsigned int sample_rate_ = 44100;
   unsigned int sample_bits_ = 16;
   int nb_channels_ = 2;

   SDL_AudioDeviceID device_ = 0;
   SDL_AudioDeviceID effects_device_ = 0;
   SDL_AudioStream* stream_ = nullptr;
   // Stream playing, or waiting for its prebuffering
   bool playing_ = false;
   unsigned int prebuffer_bytes_ = 0;
   bool sync_on_sound_ = false;

   struct Wav
   {
      SDL_AudioStream* stream = nullptr;
      Uint8* data = nullptr;
      Uint32 length = 0;
   };
   std::map<int, Wav> wav_list_;

   EmulatorEngine* emulation_ = nullptr;
   bool mute_ = false;
   float volume_ = 1.0f;
};
