#pragma once

#include "ISound.h"
#include <vector>
#include "AL/alc.h"
#include "AL/al.h"

class OAWaveHDR : public IWaveHDR
{
public:
   ALuint buffer;
};


class ALSoundMixer : public ISound
{
public:
   ALSoundMixer();
   virtual ~ALSoundMixer();

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

   // Open AL related
   ALCdevice*     device_;
   ALCcontext*    context_;

   ALuint         buffers_[NB_BUFFERS_];
   ALuint         source_;

   ALenum         format_;
};

