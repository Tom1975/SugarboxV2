#pragma once

class ISoundNotification
{
public:
   virtual void Mute(bool bMute) = 0;
   virtual bool IsMuted() = 0;
   virtual void SetVolume(float) = 0;
   virtual float GetVolume() = 0;
   virtual void Record(bool bOn) = 0;
   virtual bool IsRecordOn() = 0;
};

