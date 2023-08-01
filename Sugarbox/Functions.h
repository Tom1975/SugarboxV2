#pragma once
#include <functional>
#include <map>

#include <QAction>

#include "MultiLanguage.h"
#include "Emulation.h"
#include "IDisk.h"


class IFunctionInterface
{
public:

   class Action
   {
   public:
      QAction* action;
      std::string label_id;
      std::function<bool()> enabled;
      std::function<bool()> checked;
   };


   typedef enum
   {
      // File
      FN_EXIT,
      FN_CSL_LOAD,
      FN_SNA_LOAD,
      FN_SNA_QUICK_LOAD,
      FN_SNA_SAVE,
      FN_SNA_QUICK_SAVE,
      FN_SNR_LOAD,
      FN_SNR_RECORD,
      FN_SNR_STOP_PLAYING,
      FN_SNR_STOP_RECORD,
      // Settings
      FN_EMULATOR_SETTINGS,
      FN_CONFIG_SETTINGS,
      // Control
      FN_CTRL_ONOFF,
      FN_CTRL_PAUSE,
      FN_CTRL_SET_SPEED_10,
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
      // TAPE
      FN_TAPE_RECORD,
      FN_TAPE_PLAY,
      FN_TAPE_FASTFORWARD,
      FN_TAPE_REWIND,
      FN_TAPE_PAUSE,
      FN_TAPE_STOP,
      FN_TAPE_INSERT,
      FN_TAPE_SAVE_AS,
      // Sound
      FN_SND_RECORD,
      FN_SND_MUTE,
      // CPR
      FN_CPR_LOAD,
      // Autoload
      FN_AUTOLOAD,
      // AutoType
      FN_AUTOTYPE,
      // DISPLAY
      FN_DIS_FULLSCREEN,
      // DEBUGER
      FN_DEBUG_DEBUGGER,

   }FunctionType;

   // Get all action
   virtual Action* GetFirstAction(FunctionType&) = 0;
   virtual Action* GetNextAction(FunctionType&) = 0;

   // Action creator
   virtual QAction * GetAction(FunctionType func_type) = 0;

   // Machine state and components
   virtual bool PlusEnabled() = 0;
   virtual bool FdcPresent() = 0;
   virtual bool TapePresent() = 0;

   virtual bool IsSomethingInClipboard() = 0;
};

class Function
{
public:
   Function(MultiLanguage* multilanguage, std::string id_label, std::vector<Function*> submenu_list, std::function<bool()> available);
   Function(QAction* action, MultiLanguage* multilanguage, std::string id_label );
   virtual ~Function();

   // Node access
   bool IsNode();
   // Function access
   const char* GetLabel();
   const char* GetShortcut();
   bool IsAvailable();
   QAction* GetAction();

   unsigned int NbSubMenu();

   Function* GetMenu(int index_menu);
   void UpdateLanguage();
   
   const char* GetSubMenuLabel(int index_submenu);
   const char* GetSubMenuShortcut(int index_submenu);
   void Call(int index_submenu);
   bool IsAvailable(int index_submenu);
   bool IsSelected(int index_submenu);

protected:
   // QT Action
   QAction* action_;

   bool node_;
   std::vector<Function*> submenu_list_;
   std::string id_label_;
   std::function<bool()> available_;
   MultiLanguage* multilanguage_;
};


class FunctionList
{
public:

   FunctionList(MultiLanguage* multilanguage);
   virtual ~FunctionList();

   // Function Initialization
   void InitFunctions(IFunctionInterface* function_handler);
   void UpdateLanguage();
   void UpdateStatus();
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
