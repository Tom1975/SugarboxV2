#pragma once


class Function
{
public:
   Function();
   virtual ~Function();

   typedef enum
   {
      FN_EXIT,

   }FunctionList;

   // Functions

   // Menu


   // Label (with language)
   unsigned int id_;
   std::map<unsigned int, std::string> label_;
   std::function<void()> function_;
};

