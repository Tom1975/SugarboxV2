#pragma once

#include <string>
#include <vector>

#include <SDL3/SDL.h>

#include "AppSettings.h"
#include "IKeyboard.h"

// Joysticks of the CPC, from SDL gamepads or from keys of the PC keyboard (main thread).
// Gamepad : D-pad or left stick, A / X = fire 1, B / Y = fire 2.
class GamepadInput
{
public:
   ~GamepadInput();

   // After SDL_Init(SDL_INIT_GAMEPAD)
   void Init(IKeyboard* keyboard);
   void Shutdown();
   void Configure(const AppSettings& settings);

   // Gamepad added, removed, button, axis
   void HandleEvent(const SDL_Event& event);
   // A key of the PC keyboard : true when it is one of a joystick (not for the CPC keyboard)
   bool HandleKey(SDL_Scancode code, bool down);

   // Names of the gamepads plugged in
   std::vector<std::string> GetNames() const;

protected:
   struct Pad
   {
      SDL_JoystickID id;
      SDL_Gamepad* gamepad;
      std::string name;
   };

   void Open(SDL_JoystickID id);
   void Close(SDL_JoystickID id);
   // The gamepad of each joystick, then the state sent to the CPC
   void Update();
   unsigned int PadState(SDL_Gamepad* gamepad) const;

   IKeyboard* keyboard_ = nullptr;
   std::vector<Pad> pads_;
   AppSettings::Joystick config_[2];
   unsigned int key_state_[2] = {};
   unsigned int sent_[2] = { ~0u, ~0u };
};
