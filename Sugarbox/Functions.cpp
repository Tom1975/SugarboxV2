#include "Functions.h"





Function::Function( std::function<void()> fn) : function_(fn)
{
   
}

Function::~Function()
{
   
}

void Function::AddLabel(unsigned int language, const std::string label, const std::string shortcut)
{
   label_.insert(std::pair<unsigned int, Label>(language, { label, shortcut }));
}

const char* Function::GetLabel(unsigned int language)
{
   return label_[language].label.c_str();
}

const char* Function::GetShortcut(unsigned int language)
{
   return label_[language].shortcut.c_str();
}

void Function::Call()
{
   function_();
}

FunctionList::FunctionList():function_handler_(nullptr), current_language_(0)
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

   Function fn_exit(std::bind(&IFunctionInterface::Exit, function_handler_));
   fn_exit.AddLabel(0, "Exit", "Alt-F4");
   function_list_.insert(std::pair<IFunctionInterface::FunctionType, Function>(IFunctionInterface::FN_EXIT, fn_exit));

   // Menu init
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
   return menu_list_[index_menu].submenu_list[index_submenu]->GetLabel(current_language_);
}

const char* FunctionList::GetSubMenuShortcut(unsigned int index_menu, int index_submenu)
{
   return menu_list_[index_menu].submenu_list[index_submenu]->GetShortcut(current_language_);
}

void FunctionList::Call(unsigned int index_menu, int index_submenu)
{
   return menu_list_[index_menu].submenu_list[index_submenu]->Call();
}