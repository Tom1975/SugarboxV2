#pragma once

#include "ISound.h"
#include <vector>
#include <map>
#include <chrono>

#include "Machine.h"
#include "AL/alc.h"
#include "AL/al.h"

#include "IFrontendSound.h"

class OAWaveHDR : public IWaveHDR
{
public:
   ALuint buffer;
};


class ALSoundMixer : public IFrontendSound
{
public:
   ALSoundMixer();
   virtual ~ALSoundMixer();

   void SetEmulation(EmulatorEngine* emulation) override;

   // Interface ISoundNotification
   virtual void Mute(bool bMute);
   virtual bool IsMuted();
   virtual void SetVolume(float);
   virtual float GetVolume();
   virtual void Record(bool bOn);
   virtual bool IsRecordOn();

   // Interface ICfg
   virtual void SetDefaultConfiguration();
   virtual void SaveConfiguration(const char* config_name, const char* ini_file);
   virtual bool LoadConfiguration(const char* config_name, const char* ini_file);

   // Interface ISound
   virtual bool Init(int sample_rate, int sample_bits, int nb_channels);
   virtual void Reinit();
   virtual unsigned int GetMaxValue();
   virtual unsigned int GetMinValue();
   virtual unsigned int GetSampleRate();
   virtual unsigned int GetBitDepth();
   virtual unsigned int GetNbChannels();
   virtual void CheckBuffersStatus();

   virtual IWaveHDR* GetFreeBuffer();
   virtual void AddBufferToPlay(IWaveHDR*);

   virtual void SyncWithSound();
   virtual void SyncOnSound(bool set);
   virtual int GetPendingSamples();
   virtual void SetPrebuffer(unsigned int nb_samples);

   // Play other sound
   void AddWav(int id, const unsigned char * buffer, unsigned int size);
   void PlayWav(int wav_registered);


protected:
   // Sound values
   unsigned int sample_rate_;
   unsigned int sample_bits_;
   int nb_channels_;

   // Buffers to be filled
   static const unsigned int NB_BUFFERS_ = 25;
   // Buffer size : 1000 sample ( 1 / 44,1 of a second)
   static const unsigned int NB_SAMPLES_ = 882;

   OAWaveHDR wav_buffers_list_[NB_BUFFERS_];
   //std::vector<IWaveHDR*> list_to_play_;
   IWaveHDR* last_used_buffer_;
   bool play_;

   // Wav registered
   typedef struct
   {
      ALuint buffer;

      unsigned int format;
      unsigned int channel;
      unsigned int bps;
      int samplerate;
      unsigned int size;
      char *data;

   } WavInfo;
   std::map <int, WavInfo> wav_list_;

   // Open AL related
   ALCdevice*     device_;
   ALCcontext*    context_;

   ALuint         buffers_[NB_BUFFERS_];
   ALuint         source_;
   ALenum         format_;

   EmulatorEngine* emulation_;
   // Sound volume
   bool mute_;
   float volume_;

   bool sync_on_sound_;
   int buffer_size_for_sync_;
   // Last time the stream ran dry : the sync margin grows on an underrun, and shrinks back after
   // a long time without any
   std::chrono::steady_clock::time_point last_underrun_;

   // Prebuffering : while the stream is stopped, the buffers are held here, and handed to the
   // device all together once there are enough of them
   unsigned int prebuffer_buffers_;
   std::vector<OAWaveHDR*> held_buffers_;
   void QueueBuffer(OAWaveHDR* buffer);
   void ReleaseProcessedBuffers();
};

