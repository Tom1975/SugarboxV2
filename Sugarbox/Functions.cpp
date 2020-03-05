#include "Functions.h"



Function::Function(std::function<void()> fn, MultiLanguage* multilanguage, std::string id_label) :
id_label_(id_label), function_(fn), multilanguage_(multilanguage)
{
   
}

Function::~Function()
{
   
}

const char* Function::GetLabel()
{
   return multilanguage_->GetString(id_label_.c_str());
}

const char* Function::GetShortcut()
{
   return "TODO";
}

void Function::Call()
{
   function_();
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

   // For each function : Add languages & shortcuts
   function_list_.insert(std::pair<IFunctionInterface::FunctionType, Function>(IFunctionInterface::FN_EXIT, Function (std::bind(&IFunctionInterface::Exit, function_handler_), multilanguage_, "L_FILE_EXIT")));

   // Custom menu ?
   // Otherwise, default menu init
   menu_list_.push_back(MenuItems{ "Files",  
                  {&function_list_.at(IFunctionInterface::FN_EXIT)}
                  }
      );
}


unsigned int FunctionList::NbMenu()
{
   return menu_list_.size();
}

const char* FunctionList::MenuLabel(unsigned int index_menu)
{
   return menu_list_[index_menu].label.c_str();
}

unsigned int FunctionList::NbSubMenu(int index_menu)
{
   return menu_list_[index_menu].submenu_list.size();;
}

const char* FunctionList::GetSubMenuLabel(unsigned int index_menu, int index_submenu)
{
   return menu_list_[index_menu].submenu_list[index_submenu]->GetLabel();
}

const char* FunctionList::GetSubMenuShortcut(unsigned int index_menu, int index_submenu)
{
   return menu_list_[index_menu].submenu_list[index_submenu]->GetShortcut();
}

void FunctionList::Call(unsigned int index_menu, int index_submenu)
{
   return menu_list_[index_menu].submenu_list[index_submenu]->Call();
}