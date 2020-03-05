#pragma once
#include <functional>
#include <map>
#include "MultiLanguage.h"

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
      FN_CTRL_SET_SPEED,
      FN_CTRL_PAUSE,
      // DISK
      FN_DISK_1_EJECT,
      FN_DISK_1_INSERT,
      FN_DISK_1_FLIP,
      FN_DISK_1_SAVE_AS,
      FN_DISK_1_INSERT_BLANK,
      FN_DISK_2_EJECT,
      FN_DISK_2_INSERT,
      FN_DISK_2_FLIP,
      FN_DISK_2_SAVE_AS,
      FN_DISK_2_INSERT_BLANK,
      // Sound
      FN_SND_RECORD,
      FN_SND_MUTE
      // DISPLAY
      // DEBUGER
      // TAPE


   }FunctionType;

   virtual void Exit() = 0;

};

class Function
{
public:
   Function(std::function<void()> fn, MultiLanguage* multilanguage, std::string id_label);
   virtual ~Function();

   // Function access
   const char* GetLabel();
   const char* GetShortcut();
   void Call();

protected:

   std::string id_label_;
   std::function<void()> function_;
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
   unsigned int NbSubMenu(int index_menu);
   const char* GetSubMenuLabel(unsigned int index_menu, int index_submenu);
   const char* GetSubMenuShortcut(unsigned int index_menu, int index_submenu);
   void Call (unsigned int index_menu, int index_submenu);
   // Toolbar access

protected:
   // Direct function access
   std::map<IFunctionInterface::FunctionType, Function> function_list_;

   ///////////////////////////
   // Menu access
   struct MenuItems
   {
      std::string label;
      std::vector<Function*> submenu_list;
   };
   std::vector<MenuItems> menu_list_;
   
   // Toolbar access
   //
   //
   MultiLanguage* multilanguage_;
   IFunctionInterface* function_handler_;
};


