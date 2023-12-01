#include "Functions.h"
#include <QAction>
#include <QString>

Function::Function(MultiLanguage* multilanguage, std::string id_label, std::vector<Function*> submenu_list, std::function<bool()> available) :
   action_(nullptr), node_(true), submenu_list_(submenu_list), id_label_(id_label), available_(available), multilanguage_(multilanguage)
{

}

Function::Function(QAction* action, MultiLanguage* multilanguage, std::string id_label) :
   node_(false), multilanguage_(multilanguage), id_label_(id_label)
{
   // Create action
   action_ = action;
   action_->setText(multilanguage_->GetString(id_label.c_str()) );
}

Function::~Function()
{
   
}

QAction* Function::GetAction()
{
   return action_;
}

void Function::UpdateLanguage()
{
   action_->setText (multilanguage_->GetString(id_label_.c_str()));
}

bool Function::IsNode()
{
   return node_;
}

Function* Function::GetMenu(int index_menu)
{
   return submenu_list_[index_menu];
}

unsigned int Function::NbSubMenu()
{
   return submenu_list_.size();
}

const char* Function::GetLabel()
{
   return multilanguage_->GetString(id_label_.c_str());
}

const char* Function::GetShortcut()
{
   return "";
}

bool Function::IsAvailable()
{
   return available_();
}


FunctionList::FunctionList(MultiLanguage* multilanguage):
   multilanguage_(multilanguage),
   function_handler_(nullptr)
{

}

FunctionList::~FunctionList()
{

}

void FunctionList::InitFunctions(IFunctionInterface* function_handler)
{
   function_handler_ = function_handler;

   // Function creation
   function_list_.clear();

   IFunctionInterface::FunctionType func_type;
   const IFunctionInterface::Action* action = function_handler_->GetFirstAction(func_type);
   while (action != nullptr)
   {
      function_list_.insert(std::pair<IFunctionInterface::FunctionType, Function>(func_type, Function(action->action, multilanguage_, action->label_id)));
      action = function_handler_->GetNextAction(func_type);
   }
   
   // 
   // Custom menu ?
   // Otherwise, default menu init
   
   menu_list_.push_back(new Function(multilanguage_, "L_FN_MENU_Files", {
      &function_list_.at(IFunctionInterface::FN_EXIT),
      &function_list_.at(IFunctionInterface::FN_AUTOLOAD),
      & function_list_.at(IFunctionInterface::FN_AUTOTYPE),
      & function_list_.at(IFunctionInterface::FN_CSL_LOAD),
      },[]() { return true; }));

   
   menu_list_.push_back(new Function(multilanguage_, "L_FN_MENU_Control", 
                           {
                           &function_list_.at(IFunctionInterface::FN_CTRL_ONOFF),
                           &function_list_.at(IFunctionInterface::FN_CTRL_PAUSE),
                           new Function(multilanguage_, "L_FN_MENU_Control_Speed",
                           {
                              &function_list_.at(IFunctionInterface::FN_CTRL_SET_SPEED_10),
                              &function_list_.at(IFunctionInterface::FN_CTRL_SET_SPEED_50),
                              &function_list_.at(IFunctionInterface::FN_CTRL_SET_SPEED_100),
                              &function_list_.at(IFunctionInterface::FN_CTRL_SET_SPEED_150),
                              &function_list_.at(IFunctionInterface::FN_CTRL_SET_SPEED_200),
                              &function_list_.at(IFunctionInterface::FN_CTRL_SET_SPEED_400),
                              &function_list_.at(IFunctionInterface::FN_CTRL_SET_SPEED_VSync),
                              &function_list_.at(IFunctionInterface::FN_CTRL_SET_SPEED_MAX)
                           }, []() { return true; })
                           }, []() { return true; }));

   menu_list_.push_back(new Function(multilanguage_, "L_FN_MENU_Settings", 
                           {
                           &function_list_.at(IFunctionInterface::FN_CONFIG_SETTINGS)
                           }, []() { return true; }
                           ));

   menu_list_.push_back(new Function(multilanguage_, "L_FN_MENU_Debug",
      {
      &function_list_.at(IFunctionInterface::FN_DEBUG_DEBUGGER),
      new Function(multilanguage_, "L_DEBUG_MEMORY",
      {
         &function_list_.at(IFunctionInterface::FN_DEBUG_MEMORY_1),
         &function_list_.at(IFunctionInterface::FN_DEBUG_MEMORY_2),
         &function_list_.at(IFunctionInterface::FN_DEBUG_MEMORY_3),
         &function_list_.at(IFunctionInterface::FN_DEBUG_MEMORY_4)
      }, []() { return true; })
      }, []() { return true; }));

   menu_list_.push_back(new Function(multilanguage_, "L_FN_MENU_Disk",
      {
         new Function(multilanguage_, "L_FN_MENU_Disk_1",
         {
             &function_list_.at(IFunctionInterface::FN_DISK_1_EJECT),
             &function_list_.at(IFunctionInterface::FN_DISK_1_INSERT),
             &function_list_.at(IFunctionInterface::FN_DISK_1_FLIP),
             &function_list_.at(IFunctionInterface::FN_DISK_1_SAVE_AS),
             &function_list_.at(IFunctionInterface::FN_DISK_1_INSERT_BLANK_VENDOR),
             &function_list_.at(IFunctionInterface::FN_DISK_1_INSERT_BLANK_DATA),
         },[]() { return true; }),
         new Function(multilanguage_, "L_FN_MENU_Disk_2",
         {
             &function_list_.at(IFunctionInterface::FN_DISK_2_EJECT),
             &function_list_.at(IFunctionInterface::FN_DISK_2_INSERT),
             &function_list_.at(IFunctionInterface::FN_DISK_2_FLIP),
             &function_list_.at(IFunctionInterface::FN_DISK_2_SAVE_AS),
             &function_list_.at(IFunctionInterface::FN_DISK_2_INSERT_BLANK_VENDOR),
             &function_list_.at(IFunctionInterface::FN_DISK_2_INSERT_BLANK_DATA)
         },[]() { return true; })
      }, std::bind(&IFunctionInterface::FdcPresent, function_handler_)));

   menu_list_.push_back(new Function(multilanguage_, "L_FN_MENU_Tape",
      {
      &function_list_.at(IFunctionInterface::FN_TAPE_RECORD),
      &function_list_.at(IFunctionInterface::FN_TAPE_PLAY),
      &function_list_.at(IFunctionInterface::FN_TAPE_FASTFORWARD),
      &function_list_.at(IFunctionInterface::FN_TAPE_REWIND),
      &function_list_.at(IFunctionInterface::FN_TAPE_PAUSE),
      &function_list_.at(IFunctionInterface::FN_TAPE_STOP),
      &function_list_.at(IFunctionInterface::FN_TAPE_INSERT),
      &function_list_.at(IFunctionInterface::FN_TAPE_SAVE_AS),
      }, std::bind(&IFunctionInterface::TapePresent, function_handler_)
   ));

   menu_list_.push_back(new Function(multilanguage_, "L_FN_MENU_Sna",
      {
      &function_list_.at(IFunctionInterface::FN_SNA_LOAD),
      &function_list_.at(IFunctionInterface::FN_SNA_QUICK_LOAD),
      &function_list_.at(IFunctionInterface::FN_SNA_SAVE),
      &function_list_.at(IFunctionInterface::FN_SNA_QUICK_SAVE),
      &function_list_.at(IFunctionInterface::FN_SNR_LOAD),
      &function_list_.at(IFunctionInterface::FN_SNR_STOP_PLAYING),
      &function_list_.at(IFunctionInterface::FN_SNR_RECORD),
      &function_list_.at(IFunctionInterface::FN_SNR_STOP_RECORD)
      }, []() { return true; }));

   menu_list_.push_back(new Function(multilanguage_, "L_FN_MENU_Cpr",
      {
      &function_list_.at(IFunctionInterface::FN_CPR_LOAD)
      }, std::bind(&IFunctionInterface::PlusEnabled, function_handler_)));
      
}


unsigned int FunctionList::NbMenu()
{
   return menu_list_.size();
}

const char* FunctionList::MenuLabel(unsigned int index_menu)
{
   return menu_list_[index_menu]->GetLabel();
}

Function* FunctionList::GetMenu(int index_menu)
{
   return menu_list_[index_menu];
}

void FunctionList::UpdateLanguage()
{
   for (auto it: function_list_)
   {
      it.second.UpdateLanguage();
   }
}

void FunctionList::UpdateStatus()
{
   IFunctionInterface::FunctionType func_type;
   IFunctionInterface::Action* action = function_handler_->GetFirstAction(func_type);
   while (action != nullptr)
   {
      if ( action->checked != nullptr)
         action->action->setChecked(action->checked());
      if (action->enabled != nullptr)
         action->action->setEnabled(action->enabled());

      action = function_handler_->GetNextAction(func_type);
   }
}

