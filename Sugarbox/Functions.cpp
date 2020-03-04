#include "Functions.h"





Function::Function( std::function<void()> fn) : function_(fn)
{
   
}

Function::~Function()
{
   
}

void Function::AddLabel(unsigned int language, const std::string label)
{
   label_.insert(std::pair<unsigned int, const std::string>(language, label));
}

void FunctionList::InitFunctions(IFunctionInterface* function_handler)
{
   function_handler_ = function_handler;

   // Function creation
   function_list_.insert(std::pair<IFunctionInterface::FunctionType, Function>(IFunctionInterface::FN_EXIT, Function(std::bind(&IFunctionInterface::Exit, function_handler_))));
}
unsigned int FunctionList::NbMenu()
{
   return 1;
}

const char* FunctionList::MenuLabel(unsigned int index_menu)
{
   return "DUMMY";
}

unsigned int FunctionList::NbSubMenu(int index_menu)
{
   return 1;
}

const char* FunctionList::GetSubMenuLabel(unsigned int index_menu, int index_submenu)
{
   return "DUMMY";
}

const char* FunctionList::GetSubMenuShortcut(unsigned int index_menu, int index_submenu)
{
   return "DUMMY";
}