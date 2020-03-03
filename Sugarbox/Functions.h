#pragma once
#include <functional>
#include <map>

class IFunctionInterface
{
public:
   // Public 
   virtual void Exit() = 0;
};


class Function
{
public:
   Function(std::function<void()> fn);
   virtual ~Function();

   // Functions Init
   void AddLabel(unsigned int, const std::string);

   // Function access

protected:
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
   void InitFunctions();

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
   //
   //
   
   IFunctionInterface* function_handler_;
};


