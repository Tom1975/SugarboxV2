#include <iostream>
#include <fstream>
#include <string.h>
#include <chrono>
#include <thread>

#include "ALSoundMixer.h"

// Smallest number of buffers kept ahead when the emulation waits on the sound (20 ms each)
static const int kMinBufferForSync = 2;

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
   volume_(1.0),
   sync_on_sound_(false),
   buffer_size_for_sync_(kMinBufferForSync),
   last_underrun_(std::chrono::steady_clock::now()),
   prebuffer_buffers_(0)
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

// Called when the mixer thread (re)starts, so no buffer is being filled : the driver starts
// clean, whatever state the previous session left it in (stalled stream, queued buffers).
void ALSoundMixer::Reinit()
{
   if (context_ == nullptr) return;

   // A stopped source has processed all its buffers : they can all be taken back
   alSourceStop(source_);
   ALint nb_processed_buffers = 0;
   alGetSourcei(source_, AL_BUFFERS_PROCESSED, &nb_processed_buffers);
   if (nb_processed_buffers > 0)
   {
      ALuint buffer_to_remove[NB_BUFFERS_];
      alSourceUnqueueBuffers(source_, nb_processed_buffers < (ALint)NB_BUFFERS_ ? nb_processed_buffers : NB_BUFFERS_, buffer_to_remove);
   }
   alGetError();

   for (auto& wav_index : wav_buffers_list_)
   {
      wav_index.status_ = IWaveHDR::UNUSED;
   }
   held_buffers_.clear();
   play_ = false;
   buffer_size_for_sync_ = kMinBufferForSync;
   last_underrun_ = std::chrono::steady_clock::now();
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
   ALint nb_processed_buffers = 0;
   alGetSourcei(source_, AL_BUFFERS_PROCESSED, &nb_processed_buffers);
   ALuint buffer_to_remove[NB_BUFFERS_] ;

   alSourceUnqueueBuffers(source_, nb_processed_buffers, buffer_to_remove);
   for (int j = 0; j < nb_processed_buffers && j < NB_BUFFERS_; j++)
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

void ALSoundMixer::QueueBuffer(OAWaveHDR* oal_wav)
{
   alBufferData(oal_wav->buffer, AL_FORMAT_STEREO16, oal_wav->data_, oal_wav->buffer_length_, sample_rate_);
   alSourceQueueBuffers(source_, 1, &oal_wav->buffer);
   oal_wav->status_ = IWaveHDR::INQUEUE;
}

void ALSoundMixer::ReleaseProcessedBuffers()
{
   ALint nb_processed_buffers = 0;
   alGetSourcei(source_, AL_BUFFERS_PROCESSED, &nb_processed_buffers);
   if (nb_processed_buffers <= 0) return;
   if (nb_processed_buffers > (ALint)NB_BUFFERS_) nb_processed_buffers = NB_BUFFERS_;

   ALuint buffer_to_remove[NB_BUFFERS_];
   alSourceUnqueueBuffers(source_, nb_processed_buffers, buffer_to_remove);
   for (int j = 0; j < nb_processed_buffers; j++)
   {
      for (auto& wav_index : wav_buffers_list_)
      {
         if (wav_index.buffer == buffer_to_remove[j])
         {
            wav_index.status_ = IWaveHDR::UNUSED;
         }
      }
   }
}

void ALSoundMixer::AddBufferToPlay(IWaveHDR* new_buffer)
{
   OAWaveHDR* oal_wav = (OAWaveHDR*)new_buffer;
   ALint source_state;
   alGetSourcei(source_, AL_SOURCE_STATE, &source_state);

   if (prebuffer_buffers_ > 0 && source_state != AL_PLAYING)
   {
      // The stream is stopped (start, or ran dry) : hold the buffers until there is enough audio
      // ahead, so it restarts with its whole latency instead of running dry again at once
      oal_wav->status_ = IWaveHDR::INQUEUE;
      held_buffers_.push_back(oal_wav);
      if (held_buffers_.size() < prebuffer_buffers_)
         return;

      // A stopped source plays its queue from the start : the buffers already played leave first
      ReleaseProcessedBuffers();
      for (auto* held : held_buffers_)
      {
         QueueBuffer(held);
      }
      held_buffers_.clear();
      alSourcePlay(source_);
      play_ = true;
      return;
   }

   // Held when the prebuffering was stopped : they play first
   for (auto* held : held_buffers_)
   {
      QueueBuffer(held);
   }
   held_buffers_.clear();

   QueueBuffer(oal_wav);
   // If not launch, play the music !
   if (!play_ || source_state != AL_PLAYING)
   {
      alSourcePlay(source_);
      if (sync_on_sound_ && source_state == AL_STOPPED)
      {
         // Ran dry : keep more sound ahead
         if (buffer_size_for_sync_ < (int)NB_BUFFERS_ / 2)
            buffer_size_for_sync_++;
         last_underrun_ = std::chrono::steady_clock::now();
      }

      play_ = true;
   }
}

int ALSoundMixer::GetPendingSamples()
{
   if (context_ == nullptr) return -1;

   ALint buffer_queued = 0;
   ALint buffer_processed = 0;
   ALint sample_offset = 0;
   alGetSourcei(source_, AL_BUFFERS_QUEUED, &buffer_queued);
   alGetSourcei(source_, AL_BUFFERS_PROCESSED, &buffer_processed);
   alGetSourcei(source_, AL_SAMPLE_OFFSET, &sample_offset);
   if (alGetError() != AL_NO_ERROR) return -1;

   // The offset counts from the start of the queue (OpenAL Soft : the played buffers not
   // unqueued yet included), or from the current buffer : keep the part in the current buffer
   if (sample_offset >= buffer_processed * (ALint)NB_SAMPLES_)
      sample_offset -= buffer_processed * NB_SAMPLES_;
   if (sample_offset > (ALint)NB_SAMPLES_) sample_offset = NB_SAMPLES_;
   if (sample_offset < 0) sample_offset = 0;

   int pending = (buffer_queued - buffer_processed) * (int)NB_SAMPLES_ - sample_offset;
   pending += (int)(held_buffers_.size() * NB_SAMPLES_);
   return pending < 0 ? 0 : pending;
}

void ALSoundMixer::SetPrebuffer(unsigned int nb_samples)
{
   unsigned int nb_buffers = (nb_samples + NB_SAMPLES_ - 1) / NB_SAMPLES_;
   if (nb_buffers > NB_BUFFERS_ / 2) nb_buffers = NB_BUFFERS_ / 2;
   prebuffer_buffers_ = nb_buffers;
}

void ALSoundMixer::SyncOnSound(bool set)
{
   sync_on_sound_ = set;
}

void ALSoundMixer::SyncWithSound()
{
   // Wait until there's only xx buffer. The emulation thread waits here with the machine locked :
   // without an audio device, or when the stream stops consuming (stalled driver), it must give up,
   // or the emulation and every command (reset, snapshot) would wait forever.
   if (context_ == nullptr) return;

   ALint buffer_queued = 0;
   ALint buffer_processed = 0;
   alGetSourcei(source_, AL_BUFFERS_PROCESSED, &buffer_processed);
   alGetSourcei(source_, AL_BUFFERS_QUEUED, &buffer_queued);

   // No underrun for a long time : the margin taken after the last one can shrink back
   auto last_progress = std::chrono::steady_clock::now();
   if (buffer_size_for_sync_ > kMinBufferForSync && last_progress - last_underrun_ > std::chrono::seconds(10))
   {
      buffer_size_for_sync_--;
      last_underrun_ = last_progress;
   }

   // A buffer lasts 20 ms : no buffer played for 100 ms means the stream is stalled
   const auto stall_delay = std::chrono::milliseconds(100);
   ALint last_processed = buffer_processed;
   while (sync_on_sound_ && buffer_queued - buffer_processed > buffer_size_for_sync_)
   {
      ALint source_state = AL_STOPPED;
      alGetSourcei(source_, AL_SOURCE_STATE, &source_state);
      if (alGetError() != AL_NO_ERROR || source_state != AL_PLAYING)
         break;

      std::this_thread::sleep_for(std::chrono::microseconds(1));
      alGetSourcei(source_, AL_BUFFERS_PROCESSED, &buffer_processed);
      alGetSourcei(source_, AL_BUFFERS_QUEUED, &buffer_queued);

      const auto now = std::chrono::steady_clock::now();
      if (buffer_processed != last_processed)
      {
         last_processed = buffer_processed;
         last_progress = now;
      }
      else if (now - last_progress > stall_delay)
      {
         break;
      }
   }

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

   ALint sourceState;
   alGetSourcei(source_, AL_SOURCE_STATE, &sourceState);
   if (sourceState != AL_PLAYING)
   {
      // nothing to play !
      return 0.0f;
   }
   else
   {
      alGetSourcef(source_, AL_GAIN, &vol);
      return (float)vol;
   }

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

   alSourcePlay(source);

   ALint source_state;
   alGetSourcei(source, AL_SOURCE_STATE, &source_state);
   // check for errors
   while (source_state == AL_PLAYING) {
      alGetSourcei(source, AL_SOURCE_STATE, &source_state);
      // check for errors
   }
   // cleanup context
   alDeleteSources(1, &source);

}
