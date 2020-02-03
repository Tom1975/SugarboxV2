#pragma once

#include "ISound.h"

class SFMLSoundMixer : public ISound
{
public:
   SFMLSoundMixer();
   virtual ~SFMLSoundMixer();

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
   unsigned int sample_rate_;
   unsigned int sample_bits_;
   int nb_channels_;

};

