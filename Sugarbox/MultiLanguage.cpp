#include "MultiLanguage.h"


MultiLanguage::MultiLanguage()
{


}

MultiLanguage::~MultiLanguage()
{


}

void MultiLanguage::Init(const char* label_file)
{
   char buffer[128];

   // Load multilanguage file
   manager_.OpenFile(label_file);

   // Read all available languages
   int nb_languages = manager_.GetConfigurationInt("Languages", "Count", 1);
   for (int i = 1; i <= nb_languages; i++)
   {
      std::string key("Language_");
      key += i;
      manager_.GetConfiguration("Languages", key.c_str(), "English", buffer, sizeof(buffer));
      language_list_.push_back(buffer);
   }

   ChangeLanguage(0);
}

unsigned int MultiLanguage::GetLanguageNumber()
{
   return language_list_.size();
}

const char* MultiLanguage::GetLanguage(unsigned int index)
{
   return language_list_[index].c_str();
}

void MultiLanguage::ChangeLanguage(unsigned int index)
{
   // Load language
   current_language_.clear();


}

const char* MultiLanguage::GetString(Id_String id)
{
   return "";
}