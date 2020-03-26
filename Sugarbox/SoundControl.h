#pragma once

#include "ISoundNotification.h"

class SoundControl
{
public:
   SoundControl(ISoundNotification* sound_notification);
   virtual ~SoundControl();

   void DrawSoundVolume();

protected:
   ISoundNotification* sound_notification_;

};

