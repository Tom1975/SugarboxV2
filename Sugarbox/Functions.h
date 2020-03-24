#pragma once
#include <functional>
#include <map>
#include "MultiLanguage.h"

#include "IDisk.h"

class IFunctionInterface
{
public:
   typedef enum
   {
      // File
      FN_EXIT,
      FN_LOAD_SNA,
      FN_QUICK_LOAD_SNA,
      FN_SAVE_SNA,
      FN_QUICK_SAVE_SNA,
      // Settings
      FN_EMULATOR_SETTINGS,
      FN_CONFIG_SETTINGS,
      // Control
      FN_CTRL_ONOFF,
      FN_CTRL_PAUSE,
      FN_CTRL_SET_SPEED_50,
      FN_CTRL_SET_SPEED_100,
      FN_CTRL_SET_SPEED_150,
      FN_CTRL_SET_SPEED_200,
      FN_CTRL_SET_SPEED_400,
      FN_CTRL_SET_SPEED_VSync,
      FN_CTRL_SET_SPEED_MAX,
      // DISK
      FN_DISK_1_EJECT,
      FN_DISK_1_INSERT,
      FN_DISK_1_FLIP,
      FN_DISK_1_SAVE_AS,
      FN_DISK_1_INSERT_BLANK_VENDOR,
      FN_DISK_1_INSERT_BLANK_DATA,
      FN_DISK_2_EJECT,
      FN_DISK_2_INSERT,
      FN_DISK_2_FLIP,
      FN_DISK_2_SAVE_AS,
      FN_DISK_2_INSERT_BLANK_VENDOR,
      FN_DISK_2_INSERT_BLANK_DATA,
      // Sound
      FN_SND_RECORD,
      FN_SND_MUTE
      // DISPLAY
      // DEBUGER
      // TAPE


   }FunctionType;

   //
   virtual void Exit() = 0;

   // Control
   virtual void HardReset() = 0;
   virtual void Pause() = 0;
   virtual bool PauseEnabled() = 0;
   virtual void SetSpeed(int speedlimit) = 0;

   // Settings
   virtual void ConfigurationSettings() = 0;

   // Disk
   virtual void SaveAs(int drive) = 0;
   virtual void Eject(int drive) = 0;
   virtual bool DiskPresent(int drive) = 0;
   virtual void Flip(int drive) = 0;
   virtual void Insert(int drive) = 0;
   virtual void InsertBlank(int drive, IDisk::DiskType type) = 0;
};

class Function
{
public:
   Function(MultiLanguage* multilanguage, std::string id_label, std::vector<Function*> submenu_list);
   Function(std::function<void()> fn, MultiLanguage* multilanguage, std::string id_label, std::function<bool()> available, std::function<bool()> selected );
   virtual ~Function();

   // Node access
   bool IsNode();
   // Function access
   const char* GetLabel();
   const char* GetShortcut();
   bool IsSelected();
   bool IsAvailable();
   void Call();

   unsigned int NbSubMenu();

   Function* GetMenu(int index_menu);
   
   const char* GetSubMenuLabel(int index_submenu);
   const char* GetSubMenuShortcut(int index_submenu);
   void Call(int index_submenu);
   bool IsAvailable(int index_submenu);
   bool IsSelected(int index_submenu);

protected:
   bool node_;
   std::vector<Function*> submenu_list_;

   std::string id_label_;
   std::function<void()> function_;
   std::function<bool()> available_;
   std::function<bool()> selected_;
   MultiLanguage* multilanguage_;
};


class FunctionList
{
public:

   FunctionList(MultiLanguage* multilanguage);
   virtual ~FunctionList();

   // Function Initialization
   void InitFunctions(IFunctionInterface* function_handler);

   // Function Organization (menus / toolbar)

   // Function Access
   // Direct access
   // Menu access
   unsigned int NbMenu();
   const char* MenuLabel(unsigned int index_menu);
   Function* GetMenu(int index_menu);

protected:
   // Direct function access
   std::map<IFunctionInterface::FunctionType, Function> function_list_;

   ///////////////////////////
   // Menu access
   std::vector<Function*> menu_list_;
   
   // Toolbar access
   //
   //
   MultiLanguage* multilanguage_;
   IFunctionInterface* function_handler_;
};
