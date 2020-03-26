#pragma once

#include "ISoundNotification.h"
#include "MultiLanguage.h"

class SoundControl
{
public:
   SoundControl(ISoundNotification* sound_notification, MultiLanguage * language);
   virtual ~SoundControl();

   void DrawSoundVolume();

protected:
   ISoundNotification* sound_notification_;
   MultiLanguage* language_;
};

