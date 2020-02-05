#include "SFMLSoundMixer.h"

SFMLSoundMixer::SFMLSoundMixer():sample_rate_(0), sample_bits_(0), nb_channels_(0), play_(false), last_used_buffer_(nullptr)
{
   for (unsigned int i = 0; i < NB_BUFFERS_; i++)
   {
      wav_buffers_list_[i].data_ = nullptr;
      wav_buffers_list_[i].buffer_length_ = 0;
      wav_buffers_list_[i].status_ = IWaveHDR::UNUSED;
   }
   Init(44100, 16, 2);
}

SFMLSoundMixer::~SFMLSoundMixer()
{
   for (unsigned int i = 0; i < NB_BUFFERS_; i++)
   {
      delete []wav_buffers_list_[i].data_ ;
   }

}

///////////////////////////////////////////////////
// interface SoundStream
bool SFMLSoundMixer::onGetData(Chunk& data)
{
   data.samples = (sf::Int16*)(list_to_play_[0]->data_);
   data.sampleCount = list_to_play_[0]->buffer_length_ / sizeof (sf::Int16);
   list_to_play_[0]->status_ = IWaveHDR::USED;

   if (last_used_buffer_ != nullptr)
   {
      last_used_buffer_->status_ = IWaveHDR::UNUSED;
   }
   
   last_used_buffer_ = list_to_play_[0];
   return true;
}

void SFMLSoundMixer::onSeek(sf::Time timeOffset)
{
   
}

///////////////////////////////////////////////////
// Interface ISound
bool SFMLSoundMixer::Init(int sample_rate, int sample_bits, int nb_channels)
{
   sample_rate_ = sample_rate;
   sample_bits_ = sample_bits;
   nb_channels_ = nb_channels;

   for (unsigned int i = 0; i < NB_BUFFERS_; i++)
   {
      wav_buffers_list_[i].status_ = IWaveHDR::UNUSED;
      delete[]wav_buffers_list_[i].data_;
      wav_buffers_list_[i].buffer_length_ = NB_SAMPLES_ * nb_channels * (sample_bits >> 3);
      wav_buffers_list_[i].data_ = new char[wav_buffers_list_[i].buffer_length_];
   }
   initialize(nb_channels_, sample_rate_);
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
   for (auto& wav_index : wav_buffers_list_)
   {
      if (wav_index.status_ == IWaveHDR::UNUSED)
      {
         wav_index.status_ = IWaveHDR::USED;
         return &wav_index;
      }
   }
   return nullptr;
}

void SFMLSoundMixer::AddBufferToPlay(IWaveHDR* new_buffer)
{
   // Add to current queue.
   new_buffer->status_ = IWaveHDR::INQUEUE;
   list_to_play_.push_back(new_buffer);

   // If not launch, play the music !
   if (!play_)
   {
      play_ = true;
      play();
   }
   
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
