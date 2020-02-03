#include "SFMLSoundMixer.h"

SFMLSoundMixer::SFMLSoundMixer():sample_rate_(0), sample_bits_(0), nb_channels_(0)
{

}

SFMLSoundMixer::~SFMLSoundMixer()
{

}

// Interface ISound
bool SFMLSoundMixer::Init(int sample_rate, int sample_bits, int nb_channels)
{
   sample_rate_ = sample_rate;
   sample_bits_ = sample_bits;
   nb_channels_ = nb_channels;
   return true;
}

void SFMLSoundMixer::Reinit()
{

}

unsigned int SFMLSoundMixer::GetMaxValue()
{
   return (1 << (GetBitDepth())) - 1;
}

unsigned int SFMLSoundMixer::GetMinValue()
{
   return 0;
}

unsigned int SFMLSoundMixer::GetSampleRate()
{
   return sample_rate_;
}

unsigned int SFMLSoundMixer::GetBitDepth()
{
   return sample_bits_;
}

unsigned int SFMLSoundMixer::GetNbChannels()
{
   return nb_channels_;
}

void SFMLSoundMixer::CheckBuffersStatus()
{

}

IWaveHDR* SFMLSoundMixer::GetFreeBuffer()
{
   IWaveHDR* buffer = nullptr;

   return buffer;
}

void SFMLSoundMixer::AddBufferToPlay(IWaveHDR*)
{

}

void SFMLSoundMixer::SyncWithSound()
{

}

void SFMLSoundMixer::SetDefaultConfiguration()
{
}

void SFMLSoundMixer::SaveConfiguration(const char* config_name, const char* ini_file)
{

}

bool SFMLSoundMixer::LoadConfiguration(const char* config_name, const char* ini_file)
{
   return true;
}
