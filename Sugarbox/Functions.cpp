#include "Functions.h"


Function::Function(unsigned int id, std::function<void()> fn) : id_(id), function_(fn)
{
   
}

Function::~Function()
{
   
}

void Function::AddLabel(unsigned int language, const std::string label)
{
   label_.insert(std::pair<unsigned int, const std::string>(language, label));
}
