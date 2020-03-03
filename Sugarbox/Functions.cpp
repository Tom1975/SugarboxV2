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

void FunctionList::InitFunctions()
{
   // Function creation
   function_list_.insert(std::pair<FunctionType, Function>(FN_EXIT, Function(std::bind(&IFunctionInterface::Exit, function_handler_))));
}