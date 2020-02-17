#pragma once

#include <vector>
#include "ConfigurationManager.h"

class MultiLanguage
{
public:
   MultiLanguage();
   virtual ~MultiLanguage();

   typedef enum
   {
      Menu_Exit,

   }Id_String;

   void Init(const char* label_file);

   // Language list
   unsigned int GetLanguageNumber();
   const char* GetLanguage(unsigned int index);
   void ChangeLanguage(unsigned int index);

   const char* GetString(Id_String id);

protected:
   ConfigurationManager manager_;

   // Languages list
   std::vector<std::string> language_list_;

   // Language ids
   std::map<Id_String, std::string> current_language_;
};

