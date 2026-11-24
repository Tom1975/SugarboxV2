#include "GamepadInput.h"

namespace
{
   // Half of the stick travel
   const Sint16 kDeadZone = 16000;

   const unsigned int kKeyBits[AppSettings::KEY_COUNT] = {
      IKeyboardHandler::joy_up, IKeyboardHandler::joy_down, IKeyboardHandler::joy_left,
      IKeyboardHandler::joy_right, IKeyboardHandler::joy_but1, IKeyboardHandler::joy_but2 };
}

GamepadInput::~GamepadInput()
{
   Shutdown();
}

void GamepadInput::Init(IKeyboard* keyboard)
{
   keyboard_ = keyboard;
   int count = 0;
   SDL_JoystickID* ids = SDL_GetGamepads(&count);
   for (int i = 0; ids != nullptr && i < count; i++)
   {
      Open(ids[i]);
   }
   SDL_free(ids);
   Update();
}

void GamepadInput::Shutdown()
{
   for (auto& pad : pads_)
   {
      SDL_CloseGamepad(pad.gamepad);
   }
   pads_.clear();
}

void GamepadInput::Configure(const AppSettings& settings)
{
   for (int joy = 0; joy < 2; joy++)
   {
      config_[joy] = settings.joysticks[joy];
      key_state_[joy] = 0;
   }
   Update();
}

void GamepadInput::Open(SDL_JoystickID id)
{
   for (const auto& pad : pads_)
   {
      if (pad.id == id) return;
   }
   SDL_Gamepad* gamepad = SDL_OpenGamepad(id);
   if (gamepad == nullptr) return;
   const char* name = SDL_GetGamepadName(gamepad);
   pads_.push_back({ id, gamepad, name != nullptr ? name : "Gamepad" });
}

void GamepadInput::Close(SDL_JoystickID id)
{
   for (auto it = pads_.begin(); it != pads_.end(); ++it)
   {
      if (it->id == id)
      {
         SDL_CloseGamepad(it->gamepad);
         pads_.erase(it);
         return;
      }
   }
}

std::vector<std::string> GamepadInput::GetNames() const
{
   std::vector<std::string> names;
   for (const auto& pad : pads_)
   {
      names.push_back(pad.name);
   }
   return names;
}

void GamepadInput::HandleEvent(const SDL_Event& event)
{
   switch (event.type)
   {
   case SDL_EVENT_GAMEPAD_ADDED:
      Open(event.gdevice.which);
      break;
   case SDL_EVENT_GAMEPAD_REMOVED:
      Close(event.gdevice.which);
      break;
   case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
   case SDL_EVENT_GAMEPAD_BUTTON_UP:
   case SDL_EVENT_GAMEPAD_AXIS_MOTION:
      break;
   default:
      return;
   }
   Update();
}

bool GamepadInput::HandleKey(SDL_Scancode code, bool down)
{
   bool used = false;
   for (int joy = 0; joy < 2; joy++)
   {
      if (config_[joy].source != AppSettings::JOY_KEYBOARD) continue;
      for (int key = 0; key < AppSettings::KEY_COUNT; key++)
      {
         if (config_[joy].keys[key] != code) continue;
         if (down) key_state_[joy] |= kKeyBits[key];
         else key_state_[joy] &= ~kKeyBits[key];
         used = true;
      }
   }
   if (used) Update();
   return used;
}

unsigned int GamepadInput::PadState(SDL_Gamepad* gamepad) const
{
   unsigned int state = 0;
   const Sint16 x = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTX);
   const Sint16 y = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTY);
   if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_UP) || y < -kDeadZone) state |= IKeyboardHandler::joy_up;
   if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_DOWN) || y > kDeadZone) state |= IKeyboardHandler::joy_down;
   if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_LEFT) || x < -kDeadZone) state |= IKeyboardHandler::joy_left;
   if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_RIGHT) || x > kDeadZone) state |= IKeyboardHandler::joy_right;
   if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_SOUTH) || SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_WEST)) state |= IKeyboardHandler::joy_but1;
   if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_EAST) || SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_NORTH)) state |= IKeyboardHandler::joy_but2;
   return state;
}

void GamepadInput::Update()
{
   // Gamepad of each joystick : the one named, else the first one the other joystick does not use
   const Pad* assigned[2] = { nullptr, nullptr };
   for (int pass = 0; pass < 2; pass++)
   {
      for (int joy = 0; joy < 2; joy++)
      {
         if (config_[joy].source != AppSettings::JOY_GAMEPAD || assigned[joy] != nullptr) continue;
         // First pass : the joysticks that name their gamepad
         if ((pass == 0) == config_[joy].gamepad.empty()) continue;
         for (const auto& pad : pads_)
         {
            if (&pad == assigned[1 - joy]) continue;
            if (config_[joy].gamepad.empty() || pad.name == config_[joy].gamepad)
            {
               assigned[joy] = &pad;
               break;
            }
         }
      }
   }

   for (int joy = 0; joy < 2; joy++)
   {
      unsigned int state = 0;
      if (config_[joy].source == AppSettings::JOY_KEYBOARD) state = key_state_[joy];
      else if (assigned[joy] != nullptr) state = PadState(assigned[joy]->gamepad);

      if (state != sent_[joy] && keyboard_ != nullptr)
      {
         keyboard_->JoystickAction(joy, state);
         sent_[joy] = state;
      }
   }
}
