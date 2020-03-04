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
   void AddLabel(unsigned int, const std::string, const std::string);

   // Function access
   const char* GetLabel(unsigned int language);
   const char* GetShortcut(unsigned int language);
   void Call();

protected:
   struct Label
   {
      const std::string label;
      const std::string shortcut;
   };
   std::map<unsigned int, Label> label_;

   std::function<void()> function_;
};


class FunctionList
{
public:

   FunctionList();
   virtual ~FunctionList();

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
   void Call (unsigned int index_menu, int index_submenu);
   // Toolbar access

protected:
   // Direct function access
   std::map<IFunctionInterface::FunctionType, Function> function_list_;

   ///////////////////////////
   // Menu access
   struct SubMenuItem
   {
      std::string label;
      IFunctionInterface::FunctionType *function;
   };

   struct MenuItems
   {
      std::string label;
      std::vector<Function*> submenu_list;
   };
   std::vector<MenuItems> menu_list_;
   
   // Toolbar access
   //
   //
   
   IFunctionInterface* function_handler_;
   unsigned int current_language_;
};


