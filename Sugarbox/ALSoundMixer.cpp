#include <iostream>
#include <fstream>
#include <string.h>

#include "ALSoundMixer.h"

ALSoundMixer::ALSoundMixer():sample_rate_(0), 
   sample_bits_(0), 
   nb_channels_(0), 
   play_(false), 
   last_used_buffer_(nullptr), 
   device_(nullptr), 
   context_(nullptr),
   format_(AL_FORMAT_STEREO16),
   emulation_(nullptr),
   mute_(false),
volume_(1.0)
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

ALSoundMixer::~ALSoundMixer()
{
   // Release loaded wav !

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

void ALSoundMixer::SetEmulation(EmulatorEngine* emulation)
{
   emulation_ = emulation;
}

///////////////////////////////////////////////////
// Interface ISound
bool ALSoundMixer::Init(int sample_rate, int sample_bits, int nb_channels)
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

void ALSoundMixer::Reinit()
{

}

unsigned int ALSoundMixer::GetMaxValue()
{
   return (1 << (GetBitDepth())) - 1;
}

unsigned int ALSoundMixer::GetMinValue()
{
   return 0;
}

unsigned int ALSoundMixer::GetSampleRate()
{
   return sample_rate_;
}

unsigned int ALSoundMixer::GetBitDepth()
{
   return sample_bits_;
}

unsigned int ALSoundMixer::GetNbChannels()
{
   return nb_channels_;
}

void ALSoundMixer::CheckBuffersStatus()
{
   // Something is currently played ?
   ALint sourceState;
   alGetSourcei(source_, AL_SOURCE_STATE, &sourceState);
   if (sourceState != AL_PLAYING)
   {
      // nothing to play !
      int dbg=  1;
   }
}

IWaveHDR* ALSoundMixer::GetFreeBuffer()
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

void ALSoundMixer::AddBufferToPlay(IWaveHDR* new_buffer)
{
   ALenum error;
   OAWaveHDR* oal_wav = (OAWaveHDR*)new_buffer;
   alBufferData(oal_wav->buffer, AL_FORMAT_STEREO16, oal_wav->data_, oal_wav->buffer_length_, sample_rate_);

   alSourceQueueBuffers(source_, 1, &((OAWaveHDR*)new_buffer)->buffer);
   // If not launch, play the music !
   ALint source_state;
   alGetSourcei(source_, AL_SOURCE_STATE, &source_state);
   
   if (!play_ || source_state != AL_PLAYING)
   {          
      alSourcePlay(source_);

      play_ = true;
   }
   new_buffer->status_ = IWaveHDR::INQUEUE;

}

void ALSoundMixer::SyncWithSound()
{
   // Wait until there's only xx buffer
}

void ALSoundMixer::SetDefaultConfiguration()
{
}

void ALSoundMixer::SaveConfiguration(const char* config_name, const char* ini_file)
{

}

bool ALSoundMixer::LoadConfiguration(const char* config_name, const char* ini_file)
{
   return true;
}

void ALSoundMixer::Mute(bool bMute)
{
   if (!bMute )
   {
      if (mute_)
      {
         mute_ = false;
         SetVolume(volume_);
      }
   }
   else
   {
      if ( !mute_)
      {
         mute_ = true;
         volume_ = GetVolume();
         SetVolume(0.0);
      }
   }
}

bool ALSoundMixer::IsMuted()
{
   return mute_;
}

void ALSoundMixer::SetVolume(float vol)
{
   if (vol > 0)
      mute_ = false;
   alSourcef(source_, AL_GAIN, (ALfloat)vol);
}

float ALSoundMixer::GetVolume()
{
   ALfloat vol;
   alGetSourcef(source_, AL_GAIN, &vol);
   return (float)vol;
}

void ALSoundMixer::Record(bool bOn)
{
   if (emulation_)
   {
      if (emulation_->IsRecording())
      {
         //
         if ( !bOn)
         {
            emulation_->EndRecord();
         }
      }
      else
      {
         //
         if (bOn)
         {
            emulation_->BeginRecord();
         }
         
      }
   }
}

bool ALSoundMixer::IsRecordOn()
{
   if (emulation_)
      return (emulation_->IsRecording());
   else return false;
}


 bool isBigEndian()
{
   int a = 1;
   return !((char*)&a)[0];
}

int convertToInt(const unsigned char* buffer, int len)
{
   int a = 0;
   if (!isBigEndian())
      for (int i = 0; i < len; i++)
         ((char*)&a)[i] = buffer[i];
   else
      for (int i = 0; i < len; i++)
         ((char*)&a)[3 - i] = buffer[i];
   return a;
}

void ALSoundMixer::AddWav(int id, const unsigned char* databuffer, unsigned int buffersize)
{
   // Add new wav to wav buffer
   WavInfo info;
   
   info.channel = convertToInt(&databuffer[0x16], 2);
   info.samplerate = convertToInt(&databuffer[0x18], 4);
   info.bps = convertToInt(&databuffer[0x22], 2);
   info.size = convertToInt(&databuffer[0x28], 4);
   info.data = new char[info.size];
   memcpy(info.data, &databuffer[0x2C], info.size);

   if (info.channel == 1)
   {
      if (info.bps == 8)
      {
         info.format = AL_FORMAT_MONO8;
      }
      else {
         info.format = AL_FORMAT_MONO16;
      }
   }
   else {
      if (info.bps == 8)
      {
         info.format = AL_FORMAT_STEREO8;
      }
      else {
         info.format = AL_FORMAT_STEREO16;
      }
   }
   ALenum error;
   alGenBuffers((ALuint)1, &info.buffer);
   
   alBufferData(info.buffer, info.format, info.data, info.size, info.samplerate);

   wav_list_[id] = info;
}

void ALSoundMixer::PlayWav(int wav_registered)
{
   ALuint source;
   
   alGenSources((ALuint)1, &source);
   // check for errors

   alSourcef(source, AL_PITCH, 1);
   // check for errors
   alSourcef(source, AL_GAIN, 1);
   // check for errors
   alSource3f(source, AL_POSITION, 0, 0, 0);
   // check for errors
   alSource3f(source, AL_VELOCITY, 0, 0, 0);
   // check for errors
   alSourcei(source, AL_LOOPING, AL_FALSE);

   alSourcei(source, AL_BUFFER, wav_list_[wav_registered].buffer);

   ALenum error;
   alSourcePlay(source);

   ALint source_state;
   alGetSourcei(source, AL_SOURCE_STATE, &source_state);
   // check for errors
   while (source_state == AL_PLAYING) {
      alGetSourcei(source, AL_SOURCE_STATE, &source_state);
      // check for errors
   }
/*   // cleanup context
   alDeleteSources(1, &source);
   alDeleteBuffers(1, &buffer);
   alcMakeContextCurrent(NULL);
   */
}
