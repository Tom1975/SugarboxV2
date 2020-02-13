#include "SFMLSoundMixer.h"

SFMLSoundMixer::SFMLSoundMixer():sample_rate_(0), 
   sample_bits_(0), 
   nb_channels_(0), 
   play_(false), 
   last_used_buffer_(nullptr), 
   device_(nullptr), 
   context_(nullptr),
   format_(AL_FORMAT_STEREO16)
{
   device_ = alcOpenDevice(NULL);
   if (device_) 
   {
      context_ = alcCreateContext(device_, NULL);
      alcMakeContextCurrent(context_);
   }

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
   alcMakeContextCurrent(NULL);
   alcDestroyContext(context_);
   if (device_ != nullptr)
   {
      alcCloseDevice(device_);
   }

   for (unsigned int i = 0; i < NB_BUFFERS_; i++)
   {
      delete []wav_buffers_list_[i].data_ ;
   }

}

///////////////////////////////////////////////////
// Interface ISound
bool SFMLSoundMixer::Init(int sample_rate, int sample_bits, int nb_channels)
{
   // Generate Buffers
   alcGetError(device_); // clear error code
   alGenBuffers(NB_BUFFERS_, buffers_);

   sample_rate_ = sample_rate;
   sample_bits_ = sample_bits;
   nb_channels_ = nb_channels;

   if (nb_channels_ == 2 && sample_bits_ == 16)
      format_ = AL_FORMAT_STEREO16;
   else if (nb_channels_ == 2 && sample_bits_ == 8)
      format_ = AL_FORMAT_STEREO8;
   else if (nb_channels_ == 1 && sample_bits_ == 16)
      format_ = AL_FORMAT_MONO16;
   else if (nb_channels_ == 1 && sample_bits_ == 8)
      format_ = AL_FORMAT_MONO8;

   for (unsigned int i = 0; i < NB_BUFFERS_; i++)
   {
      wav_buffers_list_[i].buffer = buffers_[i];
      wav_buffers_list_[i].status_ = IWaveHDR::UNUSED;
      delete[]wav_buffers_list_[i].data_;
      wav_buffers_list_[i].buffer_length_ = NB_SAMPLES_ * nb_channels * (sample_bits >> 3);
      wav_buffers_list_[i].data_ = new char[wav_buffers_list_[i].buffer_length_];
   }
   
   alGenSources(1, &source_);

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
   IWaveHDR* next_buffer = nullptr;

   ALint nb_processed_buffers;
   alGetSourcei(source_, AL_BUFFERS_PROCESSED, &nb_processed_buffers);
   ALuint buffer_to_remove[NB_BUFFERS_] ;

   alSourceUnqueueBuffers(source_, nb_processed_buffers, buffer_to_remove);

   for (int j = 0; j < nb_processed_buffers; j++)
   {
      for (auto& wav_index : wav_buffers_list_)
      {
         if (wav_index.buffer == buffer_to_remove[j])
         {
            if (next_buffer == nullptr)
            {
               next_buffer = &wav_index;
               wav_index.status_ = IWaveHDR::USED;
            }
            else
            {
               wav_index.status_ = IWaveHDR::UNUSED;
            }
         }
      }
   }
            
   if (next_buffer == nullptr)
   {
      for (auto& wav_index : wav_buffers_list_)
      {
         if (wav_index.status_ == IWaveHDR::UNUSED)
         {
            wav_index.status_ = IWaveHDR::USED;
            next_buffer = &wav_index;
            break;
         }
      }
   }
   return next_buffer;
}

void SFMLSoundMixer::AddBufferToPlay(IWaveHDR* new_buffer)
{
   ALenum error;
   OAWaveHDR* oal_wav = (OAWaveHDR*)new_buffer;
   alBufferData(oal_wav->buffer, AL_FORMAT_STEREO16, oal_wav->data_, oal_wav->buffer_length_, sample_rate_);
   if ((error = alGetError()) != AL_NO_ERROR)
   {
      printf("Error : %i\n", error);
   }

   alSourceQueueBuffers(source_, 1, &((OAWaveHDR*)new_buffer)->buffer);
   if ((error = alGetError()) != AL_NO_ERROR)
   {
      printf("Error : %i\n", error);
   }
   // If not launch, play the music !
   ALint source_state;
   alGetSourcei(source_, AL_SOURCE_STATE, &source_state);
   
   if (!play_ || source_state != AL_PLAYING)
   {
      
          
      alSourcePlay(source_);
      if ((error = alGetError()) != AL_NO_ERROR)
      {
         printf("Error : %i\n", error);
      }

      play_ = true;
   }
   new_buffer->status_ = IWaveHDR::INQUEUE;

}

void SFMLSoundMixer::SyncWithSound()
{
   // Wait until there's only xx buffer
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
