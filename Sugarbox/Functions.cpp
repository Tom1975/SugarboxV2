#include "Functions.h"

Function::Function(MultiLanguage* multilanguage, std::string id_label, std::vector<Function*> submenu_list) : node_(true), multilanguage_(multilanguage), id_label_(id_label), submenu_list_(submenu_list)
{

}

Function::Function(std::function<void()> fn, MultiLanguage* multilanguage, std::string id_label, std::function<bool()> available, std::function<bool()> selected) :
   node_(false), function_(fn), multilanguage_(multilanguage), id_label_(id_label), available_(available), selected_(selected)
{
   
}

Function::~Function()
{
   
}

bool Function::IsNode()
{
   return node_;
}

Function* Function::GetMenu(int index_menu)
{
   return submenu_list_[index_menu];
}

unsigned int Function::NbSubMenu()
{
   return submenu_list_.size();
}

const char* Function::GetLabel()
{
   return multilanguage_->GetString(id_label_.c_str());
}

const char* Function::GetShortcut()
{
   return "";
}

bool Function::IsSelected()
{
   return selected_();
}

bool Function::IsAvailable()
{
   return available_();
}

void Function::Call()
{
   function_();
}

FunctionList::FunctionList(MultiLanguage* multilanguage):
   multilanguage_(multilanguage),
   function_handler_(nullptr)
{

}

FunctionList::~FunctionList()
{

}

void FunctionList::InitFunctions(IFunctionInterface* function_handler)
{
   function_handler_ = function_handler;

   // Function creation
   function_list_.clear();
   
   // For each function : Add languages & shortcuts
   function_list_.insert(std::pair<IFunctionInterface::FunctionType, Function>(IFunctionInterface::FN_EXIT, Function (std::bind(&IFunctionInterface::Exit, function_handler_), multilanguage_, "L_FILE_EXIT", []() { return true; }, []() { return false; })));
   
   // Control  
   function_list_.insert(std::pair<IFunctionInterface::FunctionType, Function>(IFunctionInterface::FN_CTRL_ONOFF, Function(std::bind(&IFunctionInterface::HardReset, function_handler_), multilanguage_, "L_CONTROL_ONOFF", []() { return true; }, []() { return false; })));
   function_list_.insert(std::pair<IFunctionInterface::FunctionType, Function>(IFunctionInterface::FN_CTRL_PAUSE, Function(std::bind(&IFunctionInterface::Pause, function_handler_), multilanguage_, "L_CONTROL_PAUSE", []() { return true; }, std::bind(&IFunctionInterface::PauseEnabled, function_handler_) )));

   function_list_.insert(std::pair<IFunctionInterface::FunctionType, Function>(IFunctionInterface::FN_CTRL_SET_SPEED_50, Function(std::bind(&IFunctionInterface::SetSpeed, function_handler_, 50), multilanguage_, "L_CONTROL_SPEED_50", []() { return true; }, []() { return false; })));
   function_list_.insert(std::pair<IFunctionInterface::FunctionType, Function>(IFunctionInterface::FN_CTRL_SET_SPEED_100, Function(std::bind(&IFunctionInterface::SetSpeed, function_handler_, 100), multilanguage_, "L_CONTROL_SPEED_100", []() { return true; }, []() { return false; })));
   function_list_.insert(std::pair<IFunctionInterface::FunctionType, Function>(IFunctionInterface::FN_CTRL_SET_SPEED_150, Function(std::bind(&IFunctionInterface::SetSpeed, function_handler_, 150), multilanguage_, "L_CONTROL_SPEED_150", []() { return true; }, []() { return false; })));
   function_list_.insert(std::pair<IFunctionInterface::FunctionType, Function>(IFunctionInterface::FN_CTRL_SET_SPEED_200, Function(std::bind(&IFunctionInterface::SetSpeed, function_handler_, 200), multilanguage_, "L_CONTROL_SPEED_200", []() { return true; }, []() { return false; })));
   function_list_.insert(std::pair<IFunctionInterface::FunctionType, Function>(IFunctionInterface::FN_CTRL_SET_SPEED_400, Function(std::bind(&IFunctionInterface::SetSpeed, function_handler_, 400), multilanguage_, "L_CONTROL_SPEED_400", []() { return true; }, []() { return false; })));
   function_list_.insert(std::pair<IFunctionInterface::FunctionType, Function>(IFunctionInterface::FN_CTRL_SET_SPEED_VSync, Function(std::bind(&IFunctionInterface::SetSpeed, function_handler_, -1), multilanguage_, "L_CONTROL_SPEED_VSYNC", []() { return true; }, []() { return false; })));
   function_list_.insert(std::pair<IFunctionInterface::FunctionType, Function>(IFunctionInterface::FN_CTRL_SET_SPEED_MAX, Function(std::bind(&IFunctionInterface::SetSpeed, function_handler_, 0), multilanguage_, "L_CONTROL_SPEED_MAX", []() { return true; }, []() { return false; })));

   // Settings
   function_list_.insert(std::pair<IFunctionInterface::FunctionType, Function>(IFunctionInterface::FN_CONFIG_SETTINGS, Function(std::bind(&IFunctionInterface::ConfigurationSettings, function_handler_), multilanguage_, "L_SETTINGS_CONFIG", []() { return true; }, []() { return false; })));
   
   // Disk
   function_list_.insert(std::pair<IFunctionInterface::FunctionType, Function>(IFunctionInterface::FN_DISK_1_SAVE_AS, Function(std::bind(&IFunctionInterface::SaveAs, function_handler_, 0), multilanguage_, "L_FN_DISK_1_SAVE_AS", std::bind(&IFunctionInterface::DiskPresent, function_handler_, 0), []() { return false; })));
   function_list_.insert(std::pair<IFunctionInterface::FunctionType, Function>(IFunctionInterface::FN_DISK_1_EJECT, Function(std::bind(&IFunctionInterface::Eject, function_handler_, 0), multilanguage_, "L_FN_DISK_1_EJECT", std::bind(&IFunctionInterface::DiskPresent, function_handler_, 0), []() { return false; })));
   function_list_.insert(std::pair<IFunctionInterface::FunctionType, Function>(IFunctionInterface::FN_DISK_1_FLIP, Function(std::bind(&IFunctionInterface::Flip, function_handler_, 0), multilanguage_, "L_FN_DISK_1_FLIP", std::bind(&IFunctionInterface::DiskPresent, function_handler_, 0), []() { return false; })));
   function_list_.insert(std::pair<IFunctionInterface::FunctionType, Function>(IFunctionInterface::FN_DISK_1_INSERT, Function(std::bind(&IFunctionInterface::Insert, function_handler_, 0), multilanguage_, "L_FN_DISK_1_INSERT", []() { return true; }, []() { return false; })));
   function_list_.insert(std::pair<IFunctionInterface::FunctionType, Function>(IFunctionInterface::FN_DISK_1_INSERT_BLANK_VENDOR, Function(std::bind(&IFunctionInterface::InsertBlank, function_handler_, 0, IDisk::VENDOR), multilanguage_, "L_FN_DISK_1_INSERT_BLANK_VENDOR", []() { return true; }, []() { return false; })));
   function_list_.insert(std::pair<IFunctionInterface::FunctionType, Function>(IFunctionInterface::FN_DISK_1_INSERT_BLANK_DATA, Function(std::bind(&IFunctionInterface::InsertBlank, function_handler_, 0, IDisk::DATA), multilanguage_, "L_FN_DISK_1_INSERT_BLANK_DATA", []() { return true; }, []() { return false; })));
   function_list_.insert(std::pair<IFunctionInterface::FunctionType, Function>(IFunctionInterface::FN_DISK_2_SAVE_AS, Function(std::bind(&IFunctionInterface::SaveAs, function_handler_, 1), multilanguage_, "L_FN_DISK_2_SAVE_AS", std::bind(&IFunctionInterface::DiskPresent, function_handler_, 1), []() { return false; })));
   function_list_.insert(std::pair<IFunctionInterface::FunctionType, Function>(IFunctionInterface::FN_DISK_2_EJECT, Function(std::bind(&IFunctionInterface::Eject, function_handler_, 1), multilanguage_, "L_FN_DISK_2_EJECT", std::bind(&IFunctionInterface::DiskPresent, function_handler_, 1), []() { return false; })));
   function_list_.insert(std::pair<IFunctionInterface::FunctionType, Function>(IFunctionInterface::FN_DISK_2_FLIP, Function(std::bind(&IFunctionInterface::Flip, function_handler_, 1), multilanguage_, "L_FN_DISK_2_FLIP", std::bind(&IFunctionInterface::DiskPresent, function_handler_, 1), []() { return false; })));
   function_list_.insert(std::pair<IFunctionInterface::FunctionType, Function>(IFunctionInterface::FN_DISK_2_INSERT, Function(std::bind(&IFunctionInterface::Insert, function_handler_, 1), multilanguage_, "L_FN_DISK_2_INSERT", []() { return true; }, []() { return false; })));
   function_list_.insert(std::pair<IFunctionInterface::FunctionType, Function>(IFunctionInterface::FN_DISK_2_INSERT_BLANK_VENDOR, Function(std::bind(&IFunctionInterface::InsertBlank, function_handler_, 1, IDisk::VENDOR), multilanguage_, "L_FN_DISK_2_INSERT_BLANK_VENDOR", []() { return true; }, []() { return false; })));
   function_list_.insert(std::pair<IFunctionInterface::FunctionType, Function>(IFunctionInterface::FN_DISK_2_INSERT_BLANK_DATA, Function(std::bind(&IFunctionInterface::InsertBlank, function_handler_, 1, IDisk::DATA), multilanguage_, "L_FN_DISK_2_INSERT_BLANK_DATA", []() { return true; }, []() { return false; })));

   // Tape
   function_list_.insert(std::pair<IFunctionInterface::FunctionType, Function>(IFunctionInterface::FN_TAPE_RECORD, Function(std::bind(&IFunctionInterface::TapeRecord, function_handler_), multilanguage_, "L_FN_TAPE_RECORD", []() { return true; }, []() { return false; })));
   function_list_.insert(std::pair<IFunctionInterface::FunctionType, Function>(IFunctionInterface::FN_TAPE_PLAY, Function(std::bind(&IFunctionInterface::TapePlay, function_handler_), multilanguage_, "L_FN_TAPE_PLAY", []() { return true; }, []() { return false; })));
   function_list_.insert(std::pair<IFunctionInterface::FunctionType, Function>(IFunctionInterface::FN_TAPE_FASTFORWARD, Function(std::bind(&IFunctionInterface::TapeFastForward, function_handler_), multilanguage_, "L_FN_TAPE_FASTFORWARD", []() { return true; }, []() { return false; })));
   function_list_.insert(std::pair<IFunctionInterface::FunctionType, Function>(IFunctionInterface::FN_TAPE_REWIND, Function(std::bind(&IFunctionInterface::TapeRewind, function_handler_), multilanguage_, "L_FN_TAPE_REWIND", []() { return true; }, []() { return false; })));
   function_list_.insert(std::pair<IFunctionInterface::FunctionType, Function>(IFunctionInterface::FN_TAPE_PAUSE, Function(std::bind(&IFunctionInterface::TapePause, function_handler_), multilanguage_, "L_FN_TAPE_PAUSE", []() { return true; }, []() { return false; })));
   function_list_.insert(std::pair<IFunctionInterface::FunctionType, Function>(IFunctionInterface::FN_TAPE_STOP, Function(std::bind(&IFunctionInterface::TapeStop, function_handler_), multilanguage_, "L_FN_TAPE_STOP", []() { return true; }, []() { return false; })));
   function_list_.insert(std::pair<IFunctionInterface::FunctionType, Function>(IFunctionInterface::FN_TAPE_INSERT, Function(std::bind(&IFunctionInterface::TapeInsert, function_handler_), multilanguage_, "L_FN_TAPE_INSERT", []() { return true; }, []() { return false; })));
   function_list_.insert(std::pair<IFunctionInterface::FunctionType, Function>(IFunctionInterface::FN_TAPE_SAVE_AS_WAV, Function(std::bind(&IFunctionInterface::TapeSaveAs, function_handler_, Emulation::TAPE_WAV), multilanguage_, "L_FN_TAPE_SAVE_AS_WAV", []() { return true; }, []() { return false; })));
   function_list_.insert(std::pair<IFunctionInterface::FunctionType, Function>(IFunctionInterface::FN_TAPE_SAVE_AS_CDT_DRB, Function(std::bind(&IFunctionInterface::TapeSaveAs, function_handler_, Emulation::TAPE_CDT_DRB), multilanguage_, "L_FN_TAPE_SAVE_AS_CDT_DRB", []() { return true; }, []() { return false; })));
   function_list_.insert(std::pair<IFunctionInterface::FunctionType, Function>(IFunctionInterface::FN_TAPE_SAVE_AS_CDT_CSW, Function(std::bind(&IFunctionInterface::TapeSaveAs, function_handler_, Emulation::TAPE_CDT_CSW), multilanguage_, "L_FN_TAPE_SAVE_AS_CDT_CSW", []() { return true; }, []() { return false; })));
   function_list_.insert(std::pair<IFunctionInterface::FunctionType, Function>(IFunctionInterface::FN_TAPE_SAVE_AS_CSW11, Function(std::bind(&IFunctionInterface::TapeSaveAs, function_handler_, Emulation::TAPE_CSW11), multilanguage_, "L_FN_TAPE_SAVE_AS_CSW11", []() { return true; }, []() { return false; })));
   function_list_.insert(std::pair<IFunctionInterface::FunctionType, Function>(IFunctionInterface::FN_TAPE_SAVE_AS_CSW20, Function(std::bind(&IFunctionInterface::TapeSaveAs, function_handler_, Emulation::TAPE_CSW20), multilanguage_, "L_FN_TAPE_SAVE_AS_CSW20", []() { return true; }, []() { return false; })));

   // Snapshots
   function_list_.insert(std::pair<IFunctionInterface::FunctionType, Function>(IFunctionInterface::FN_SNA_LOAD, Function(std::bind(&IFunctionInterface::SnaLoad, function_handler_), multilanguage_, "L_FN_LOAD_SNA", []() { return true; }, []() { return false; })));


   // Custom menu ?
   // Otherwise, default menu init
   
   menu_list_.push_back(new Function(multilanguage_, "L_FN_MENU_Files", {
      &function_list_.at(IFunctionInterface::FN_EXIT)
      }));
   
   menu_list_.push_back(new Function(multilanguage_, "L_FN_MENU_Control", 
                           {
                           &function_list_.at(IFunctionInterface::FN_CTRL_ONOFF),
                           &function_list_.at(IFunctionInterface::FN_CTRL_PAUSE),
                           new Function(multilanguage_, "L_FN_MENU_Control_Speed",
                           {
                              &function_list_.at(IFunctionInterface::FN_CTRL_SET_SPEED_50),
                              &function_list_.at(IFunctionInterface::FN_CTRL_SET_SPEED_100),
                              &function_list_.at(IFunctionInterface::FN_CTRL_SET_SPEED_150),
                              &function_list_.at(IFunctionInterface::FN_CTRL_SET_SPEED_200),
                              &function_list_.at(IFunctionInterface::FN_CTRL_SET_SPEED_400),
                              &function_list_.at(IFunctionInterface::FN_CTRL_SET_SPEED_VSync),
                              &function_list_.at(IFunctionInterface::FN_CTRL_SET_SPEED_MAX)
                           })
                           }));
   
   menu_list_.push_back(new Function(multilanguage_, "L_FN_MENU_Settings", 
                           {
                           &function_list_.at(IFunctionInterface::FN_CONFIG_SETTINGS)}
                           ));

   menu_list_.push_back(new Function(multilanguage_, "L_FN_MENU_Disk",
      {
         new Function(multilanguage_, "L_FN_MENU_Disk_1",
         {
             &function_list_.at(IFunctionInterface::FN_DISK_1_EJECT),
             &function_list_.at(IFunctionInterface::FN_DISK_1_INSERT),
             &function_list_.at(IFunctionInterface::FN_DISK_1_FLIP),
             &function_list_.at(IFunctionInterface::FN_DISK_1_SAVE_AS),
             &function_list_.at(IFunctionInterface::FN_DISK_1_INSERT_BLANK_VENDOR),
             &function_list_.at(IFunctionInterface::FN_DISK_1_INSERT_BLANK_DATA),
         }),
         new Function(multilanguage_, "L_FN_MENU_Disk_2",
         {
             &function_list_.at(IFunctionInterface::FN_DISK_2_EJECT),
             &function_list_.at(IFunctionInterface::FN_DISK_2_INSERT),
             &function_list_.at(IFunctionInterface::FN_DISK_2_FLIP),
             &function_list_.at(IFunctionInterface::FN_DISK_2_SAVE_AS),
             &function_list_.at(IFunctionInterface::FN_DISK_2_INSERT_BLANK_VENDOR),
             &function_list_.at(IFunctionInterface::FN_DISK_2_INSERT_BLANK_DATA)
         })
      }));

   menu_list_.push_back(new Function(multilanguage_, "L_FN_MENU_Tape",
      {
      &function_list_.at(IFunctionInterface::FN_TAPE_RECORD),
      &function_list_.at(IFunctionInterface::FN_TAPE_PLAY),
      &function_list_.at(IFunctionInterface::FN_TAPE_FASTFORWARD),
      &function_list_.at(IFunctionInterface::FN_TAPE_REWIND),
      &function_list_.at(IFunctionInterface::FN_TAPE_PAUSE),
      &function_list_.at(IFunctionInterface::FN_TAPE_STOP),
      &function_list_.at(IFunctionInterface::FN_TAPE_INSERT),
      new Function(multilanguage_, "L_FN_MENU_SAVE_TAPE",
         {
            &function_list_.at(IFunctionInterface::FN_TAPE_SAVE_AS_WAV),
            &function_list_.at(IFunctionInterface::FN_TAPE_SAVE_AS_CDT_DRB),
            &function_list_.at(IFunctionInterface::FN_TAPE_SAVE_AS_CDT_CSW),
            &function_list_.at(IFunctionInterface::FN_TAPE_SAVE_AS_CSW11),
            &function_list_.at(IFunctionInterface::FN_TAPE_SAVE_AS_CSW20),
         })
      }
   ));
   menu_list_.push_back(new Function(multilanguage_, "L_FN_MENU_Sna",
      {
      &function_list_.at(IFunctionInterface::FN_SNA_LOAD)
      }));
}


unsigned int FunctionList::NbMenu()
{
   return menu_list_.size();
}

const char* FunctionList::MenuLabel(unsigned int index_menu)
{
   return menu_list_[index_menu]->GetLabel();
}

Function* FunctionList::GetMenu(int index_menu)
{
   return menu_list_[index_menu];
}
