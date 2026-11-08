#pragma once

#include "ISound.h"
#include "ISoundNotification.h"

class EmulatorEngine;

// Sound output of a frontend : the stream of the emulated machine (ISound), the sounds of the disk
// drives, and the user controls (volume, mute, record). The emulation only knows this interface,
// so every frontend (Qt with OpenAL, SDL) brings its own.
class IFrontendSound : public ISound, public ISoundNotification
{
public:
   virtual ~IFrontendSound() = default;

   // The engine recorded by Record()
   virtual void SetEmulation(EmulatorEngine* emulation) = 0;

   // Sounds of the drives : a whole .wav file, registered once, played on demand
   virtual void AddWav(int id, const unsigned char* buffer, unsigned int size) = 0;
   virtual void PlayWav(int wav_registered) = 0;
};
