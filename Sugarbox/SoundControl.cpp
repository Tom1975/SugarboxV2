#include "SoundControl.h"

SoundControl::SoundControl(ISoundNotification* sound_notification, MultiLanguage* language) :sound_notification_(sound_notification), language_(language)
{
}

SoundControl::~SoundControl()
{

}

void SoundControl::DrawSoundVolume()
{
   float v = sound_notification_->GetVolume();

  
};