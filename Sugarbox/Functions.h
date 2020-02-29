#pragma once
#include <functional>
#include <map>

class Function
{
public:
   Function(unsigned int id, std::function<void()> fn);
   virtual ~Function();

   // Functions Init
   void AddLabel(unsigned int, const std::string);

   // Function access

protected:
   unsigned int id_;
   std::map<unsigned int, const std::string> label_;
   std::function<void()> function_;
};


class FunctionList
{
public:
   typedef enum
   {
      FN_EXIT,

   }FunctionType;

   // Function Initialization
   // Function Organization (menus / toolbar)

   // Function Access
   // Direct access
   // Menu access 
   // Toolbar access

protected:
   // Direct function access
   std::map<FunctionType, Function> function_list_;
   // Menu access
   // Toolbar access
};


