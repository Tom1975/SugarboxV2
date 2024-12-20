#pragma once

class SettingsValues
{
public:

   enum Color
   {
      BACK_COLOR,
      MARGIN_COLOR,
      ADDRESS_COLOR,
      MNEMONIC_COLOR,
      ARGUMENT_COLOR,
      BYTE_COLOR,
      CHAR_COLOR,
      SELECTION_COLOR,
      DRIVE_STATUS_NODISK_1,
      DRIVE_STATUS_NODISK_2,
      DRIVE_STATUS_NO_ACTIVITY_1,
      DRIVE_STATUS_NO_ACTIVITY_2,
      DRIVE_STATUS_READ_1,
      DRIVE_STATUS_READ_2,
      DRIVE_STATUS_WRITE_1,
      DRIVE_STATUS_WRITE_2,
      EDIT_BACK,
      EDIT_TEXT,
      EDIT_TEXT_DISABLED,
      EDIT_TEXT_CHANGED,
   };

   enum FontType
   {
      DISASSEMBLY_FONT,
      REGISTER_FONT,
      MEMORY_FONT,
   };

   enum ActionType
   {
      DBG_RUN_ACTION,
      DBG_BREAK_ACTION,
      DBG_STEP_ACTION,
      DBG_STEPIN_ACTION,
      DBG_STEPOUT_ACTION,
      DBG_TOGGLE_FLAG_ACTION,
      DBG_TOGGLE_BREAKPOINT_ACTION,

   };
};
