#pragma once
#include <functional>
#include <map>

class IFunctionInterface
{
public:
   typedef enum
   {
      FN_EXIT,

   }FunctionType;

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

   // Toolbar access

protected:
   // Direct function access
   std::map<IFunctionInterface::FunctionType, Function> function_list_;
   // Menu access
   
   
   // Toolbar access
   //
   //
   
   IFunctionInterface* function_handler_;
};


