#include <cstring>

#include "ConfigurationManager.h"

/////////////////////////////////////////////////////////////
/// Helper functions
ConfigurationManager::ConfigurationManager()
{
}

ConfigurationManager::~ConfigurationManager()
{
   // Clear everything
   Clear();
}

void ConfigurationManager::Clear()
{
   for (auto const& ent1 : config_file_)
   {
      // ent1.first is the first key
      if (ent1.second != nullptr)
      { 
         for (auto const& ent2 : *ent1.second)
         {
            // ent2.first is the second key
            // ent2.second is the data
         }

      }
      ent1.second->clear();
   }
   config_file_.clear();
}

void ConfigurationManager::OpenNewFile(const char* config_file)
{
   current_config_file_ = config_file;
}

void ConfigurationManager::OpenFile(const char* config_file)
{
   if (current_config_file_ == config_file)
   {
      // already openend
      return;
   }
   Clear();
   std::string s, key, value;
   std::ifstream f(config_file);
   std::string current_section = "";

   current_config_file_ = config_file;

   while (std::getline(f, s))
   {
      std::string::size_type begin = s.find_first_not_of(" \f\t\v");

      // Skip blank lines
      if (begin == std::string::npos) continue;
      // Skip commentary
      if (std::string("#;").find(s[begin]) != std::string::npos) continue;

      // Search sections
      //std::string::size_type begin_section = s.find_first_of("[");
      //if (begin_section != std::string::npos)
      if (s[0] == '[')
      {
         std::string::size_type end_section = s.find_first_of("]");
         if (end_section != std::string::npos)
         {
            current_section = s.substr(1, end_section - 1);
         }
      }

      // Search key (if a section is already defined)
      if (current_section.size() > 0)
      {
         // Extract the key value
         std::string::size_type end = s.find('=', begin);

         if (end == std::string::npos) continue;

         key = s.substr(begin, end - begin);

         // (No leading or trailing whitespace allowed)
         key.erase(key.find_last_not_of(" \f\t\v") + 1);

         // No blank keys allowed
         if (key.empty()) continue;

         // Extract the value (no leading or trailing whitespace allowed)
         begin = s.find_first_not_of(" \f\n\r\t\v", end + 1);
         if (begin == std::string::npos)
         {
            value = "";
         }
         else
         {
            end = s.find_last_not_of(" \f\n\r\t\v") + 1;
            value = s.substr(begin, end - begin);
         }



         // Add this key/value to current section
         data* d;
         if (config_file_.count(current_section) > 0)
         {
            d = config_file_.at(current_section);
         }
         else
         {
            d = new data;
            config_file_[current_section] = d;
         }
         (*d)[key] = value;
      }
   }
}

void ConfigurationManager::CloseFile()
{
   std::ofstream f(current_config_file_);

   // Write this file
   for (auto const& ent1 : config_file_)
   {
      // ent1.first is the first key
      if (ent1.second != nullptr)
      {
         f << "[" << ent1.first << "]\n";

         for (auto const& ent2 : *ent1.second)
         {
            // ent2.first is the second key
            f << ent2.first << " = " << ent2.second << "\n";
            // ent2.second is the data
         }

      }
   }

}

void ConfigurationManager::SetConfiguration(const char* section, const char* key, const char* value, const char* file)
{
   OpenFile(file);
   SetConfiguration(section, key, value);
}
void ConfigurationManager::SetConfiguration(const char* section, const char* key, const char* value)
{
   // Add or update key
   data* d;
   if (config_file_.count(section) == 0)
   {
      d = new data;
      config_file_[section] = d;
   }
   else
   {
      d = config_file_[section];
   }
   (*d)[key] = value;
   config_file_.at(section) = d;
   // rewrite whole file
}

unsigned int ConfigurationManager::GetConfiguration(const char* section, const char* cle, const char* default_value, char* out_buffer, unsigned int buffer_size, const char* file)
{
   OpenFile(file);
   return GetConfiguration(section, cle, default_value, out_buffer, buffer_size);
}

unsigned int ConfigurationManager::GetConfiguration(const char* section, const char* cle, const char* default_value, char* out_buffer, unsigned int buffer_size)
{
   if (config_file_.count(section) > 0)
   {
      if (config_file_[section]->count(cle) > 0)
      {
         std::string value = config_file_[section]->at(cle);
         if (value.size() < buffer_size)
         {
            std::strncpy(out_buffer, value.c_str(), buffer_size);
            return strlen(out_buffer);
         }
         else
         {
            std::strncpy(out_buffer, value.c_str(), buffer_size);
            return buffer_size;
         }
      }
   }
   std::strncpy(out_buffer, default_value, buffer_size);
   return 0;
}

unsigned int ConfigurationManager::GetConfigurationInt(const char* section, const char* cle, unsigned int default_value, const char* file)
{
   OpenFile(file);
   return GetConfigurationInt(section, cle, default_value);
}

unsigned int ConfigurationManager::GetConfigurationInt(const char* section, const char* cle, unsigned int default_value)
{
   if (config_file_.count(section) > 0)
   {
      if (config_file_[section]->count(cle) > 0)
      {
         std::string value = config_file_[section]->at(cle);
         return atoi(value.c_str());
      }
   }
   return default_value;
}

const char* ConfigurationManager::GetFirstSection()
{
   it_section_ = config_file_.begin();
   return GetNextSection();
}

const char* ConfigurationManager::GetNextSection()
{
   if (it_section_ != config_file_.end())
   {
      const char* value = it_section_->first.c_str();
      ++it_section_;
      return value;
   }
   return nullptr;
}

const char* ConfigurationManager::GetFirstKey(const char* section)
{
   current_key_section_it_ = config_file_[std::string(section)];
   if (current_key_section_it_ == nullptr )
      return nullptr;
   it_key_ = current_key_section_it_->begin();
   return GetNextKey();
}

const char* ConfigurationManager::GetNextKey()
{
   if (it_key_ != current_key_section_it_->end())
   {
      const char* value = it_key_->first.c_str();
      ++it_key_;
      return value;
   }
   return nullptr;
}

