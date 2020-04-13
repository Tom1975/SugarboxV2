#include "SugarboxApp.h"
#include "ui_SugarboxApp.h"

#include <filesystem>

/////////////////////////////////////
// SugarbonApp

SugarboxApp::SugarboxApp(QWidget *parent) : QMainWindow(parent), counter_(0), str_speed_("0%"), write_disk_extension_(nullptr), 
load_disk_extension_(nullptr), keyboard_handler_(nullptr), language_(), functions_list_(&language_),
dlg_settings_(&config_manager_), sound_control_(&sound_mixer_, &language_), configuration_settings_(false)/*,
ui(new Ui::SugarboxApp)*/
{

   emulation_ = new Emulation();
   widget_ = new QWidget();
   setCentralWidget(widget_);

   mainLayout_ = new QVBoxLayout();

   QWidget *top_filler = new QWidget;
   QWidget *bottom_filler = new QWidget;
   //mainLayout_->addWidget(top_filler);
   mainLayout_->addWidget(&display_);
   //mainLayout_->addWidget(bottom_filler);


   widget_->setLayout(mainLayout_);
   setWindowTitle(tr("SugarboxV2"));

   setMinimumSize(160, 160);
   resize(800, 600);
}

SugarboxApp::~SugarboxApp()
{
   delete emulation_;
   delete[]write_disk_extension_;
   delete[]load_disk_extension_;
}

//////////////////////////////////////////////
// QT functions
void SugarboxApp::createMenus ()
{
   menu_list_.clear();
   for (unsigned int menu_index = 0; menu_index < functions_list_.NbMenu(); menu_index++)
   {
      if (functions_list_.GetMenu(menu_index)->IsAvailable())
      {
         QMenu* menu = menuBar()->addMenu(tr(functions_list_.GetMenu(menu_index)->GetLabel()));
         menu_list_.push_back(menu);
         DrawSubMenu(menu, functions_list_.GetMenu(menu_index), true);
      }
   }   
}

void SugarboxApp::DrawSubMenu(QMenu* menu, Function* function, bool toplevel)
{
   if (function->IsNode())
   {
      QMenu* submenu;
      if (toplevel)
      {
         submenu = menu;
      }
      else
      {
         submenu = menu->addMenu(tr(function->GetLabel()));
         menu_list_.push_back(submenu);
      }
      for (unsigned int submenu_index = 0; submenu_index < function->NbSubMenu(); submenu_index++)
      {
         Function* subfunction = function->GetMenu(submenu_index);
         DrawSubMenu(submenu, function->GetMenu(submenu_index));
      }
   }
   else
   {
      // Item
      menu->addAction(function->GetAction());
   }
}
//////////////////////////////////////////////
/// ISoundFactory interface
ISound* SugarboxApp::GetSound(const char* name)
{
   return (ISound * )&sound_mixer_;
}

const char* SugarboxApp::GetSoundName(ISound*)
{
   return "SFML sound mixer";
}

const char* SugarboxApp::GetFirstSoundName()
{
   return "SFML sound mixer";
}

const char* SugarboxApp::GetNextSoundName()
{
   return nullptr;
}

void SugarboxApp::InitMenu()
{
   InitAllActions();
   functions_list_.InitFunctions(this);
   language_.Init("Resources/labels.ini");
   functions_list_.UpdateLanguage();
   InitFileDialogs();
}

int SugarboxApp::RunApp()
{
   std::filesystem::path current_path_exe = std::filesystem::current_path();

   // Create the main window
   window_width_ = main_display_width + peripherals_width;
   window_height_ = main_display_height + toolbar_height + status_height;

   // Init display
   SizeChanged(window_width_, window_height_);

   display_.Init();
   emulation_->Init(&display_, this, &sound_mixer_, current_path_exe.string().c_str());
   dlg_settings_.Init(emulation_->GetEngine());
   keyboard_handler_ = emulation_->GetKeyboardHandler();
   
   // Get current directory, and add the CONF to it
   current_path_exe /= "CONF";
   dlg_settings_.Refresh(current_path_exe.string().c_str());

   InitMenu();
   createMenus();


   /*

   // This part was used to convert keyboard from windows to keycode (for cross platform usage)
   // It's no longer used, but I think I don't want to lose this code :)
   // Set a map to convert scancode to key
   std::map < int, int > scancode_to_key;
   for (int k = GLFW_KEY_SPACE; k <= GLFW_KEY_LAST; k++)
   {
      scancode_to_key.insert(std::pair<int, int>(glfwGetKeyScancode(k), k));
   }
   ConfigurationManager key_mgr, key_mgr_out;
   std::filesystem::path key_path = current_path_exe;
   key_path /= "KeyboardMaps.ini";
   key_mgr.OpenFile(key_path.string().c_str());

   std::filesystem::path key_path_out = current_path_exe;
   key_path_out /= "KeyboardMaps_Generic.ini";
   key_mgr_out.OpenFile(key_path_out.string().c_str());

   // Load KeyboardMaps.ini
   // Read scancode, convert to key
   // Write it
   const char* section = key_mgr.GetFirstSection();
   while (section != nullptr)
   {
      const char* key = key_mgr.GetFirstKey(section);
      while (key != nullptr)
      {
         if (strstr(key, "_SC") != nullptr)
         {
            // Ok, this is a scancode key.
            unsigned int value_sc = key_mgr.GetConfigurationInt(section, key, -1);

            // Convert it
            unsigned int value_key = scancode_to_key[value_sc];

            if (value_key != 0)
            {
               // Add it to new map
               char buffer[16];
               sprintf(buffer, "%i", value_key);
               key_mgr_out.SetConfiguration(section, key, buffer);

               // Also, get the other keys : 
               char char_buffer[16];
               memset(char_buffer, 0, sizeof(char_buffer));
               char base_key[32];
               memset(base_key, 0, sizeof(base_key));
               strncpy(base_key, key, 4);

               // char, uchar, charctrl, ucharctrl
               strcpy(&base_key[4], "char"); 
               key_mgr.GetConfiguration(section, base_key, "", char_buffer, 16);
               key_mgr_out.SetConfiguration(section, base_key, char_buffer);

               strcpy(&base_key[4], "char_value");
               key_mgr.GetConfiguration(section, base_key, "", char_buffer, 16);
               key_mgr_out.SetConfiguration(section, base_key, char_buffer);

               strcpy(&base_key[4], "char_value_Alt");
               key_mgr.GetConfiguration(section, base_key, "", char_buffer, 16);
               key_mgr_out.SetConfiguration(section, base_key, char_buffer);

               strcpy(&base_key[4], "UCHAR");
               key_mgr.GetConfiguration(section, base_key, "", char_buffer, 16);
               key_mgr_out.SetConfiguration(section, base_key, char_buffer);

               strcpy(&base_key[4], "UCHAR_value");
               key_mgr.GetConfiguration(section, base_key, "", char_buffer, 16);
               key_mgr_out.SetConfiguration(section, base_key, char_buffer);

               strcpy(&base_key[4], "UCHAR_Alt");
               key_mgr.GetConfiguration(section, base_key, "", char_buffer, 16);
               key_mgr_out.SetConfiguration(section, base_key, char_buffer);

               strcpy(&base_key[4], "UCHAR_value_Alt");
               key_mgr.GetConfiguration(section, base_key, "", char_buffer, 16);
               key_mgr_out.SetConfiguration(section, base_key, char_buffer);
               
               strcpy(&base_key[4], "charCtrl");
               key_mgr.GetConfiguration(section, base_key, "", char_buffer, 16);
               key_mgr_out.SetConfiguration(section, base_key, char_buffer);

               strcpy(&base_key[4], "charCtrl_value");
               key_mgr.GetConfiguration(section, base_key, "", char_buffer, 16);
               key_mgr_out.SetConfiguration(section, base_key, char_buffer);
               
               strcpy(&base_key[4], "UCHARCTRL");
               key_mgr.GetConfiguration(section, base_key, "", char_buffer, 16);
               key_mgr_out.SetConfiguration(section, base_key, char_buffer);

               strcpy(&base_key[4], "UCHARCTRL_value");
               key_mgr.GetConfiguration(section, base_key, "", char_buffer, 16);
               key_mgr_out.SetConfiguration(section, base_key, char_buffer);
               
            }
         }

         key = key_mgr.GetNextKey();
      }
      section = key_mgr.GetNextSection();
   }
   key_mgr_out.CloseFile();
   */

   // Run main loop
   //RunMainLoop();

   return 0;
}

static bool test = false;

void SugarboxApp::RunMainLoop()
{
   while (true)
   {
      DrawMainWindow();

      DrawMenu();
      DrawPeripherals();
      DrawStatusBar();
      DrawOthers();


   }

   emulation_->Stop();
}

void SugarboxApp::DrawMainWindow()
{
   // Draw texture
   display_.Display();
}

void SugarboxApp::DrawMenu()
{

}

void SugarboxApp::DrawPeripherals()
{
}

void SugarboxApp::DrawStatusBar()
{
   // Speed of emulation : only 1 every 50 frames
   counter_++;

   if (counter_ == 50 /*|| m_pMachine->GetMonitor()->m_bSpeed*/)
   {
      // Update
   
      sprintf(str_speed_, "%i %%%%", emulation_->GetSpeed());
      counter_ = 0;
   }
   sound_control_.DrawSoundVolume();
}

void SugarboxApp::DrawOthers()
{
   // Configuration ?
   
   if (configuration_settings_)
   {
      dlg_settings_.DisplayMenu();
   }
}

void SugarboxApp::SizeChanged(int width, int height)
{
   //glViewport(0, height - toolbar_height - main_display_height, main_display_width, main_display_height);
}

void SugarboxApp::Drop(int count, const char** paths)
{
   // Check for headers :
   for (int i = 0; i < 4 && i < count; i++)
   {
      DataContainer* dnd_container = emulation_->CanLoad(paths[i]);

      MediaManager mediaMgr(dnd_container);
      std::vector<MediaManager::MediaType> list_of_types;
      list_of_types.push_back(MediaManager::MEDIA_DISK);
      list_of_types.push_back(MediaManager::MEDIA_SNA);
      list_of_types.push_back(MediaManager::MEDIA_SNR);
      list_of_types.push_back(MediaManager::MEDIA_TAPE);
      list_of_types.push_back(MediaManager::MEDIA_BIN);
      list_of_types.push_back(MediaManager::MEDIA_CPR);

      int media_type = mediaMgr.GetType(list_of_types);

      switch (media_type)
      {
         // Test : Is it SNA?
      case 1:
         emulation_->LoadSnapshot(paths[0]);
         break;
      case 2:
         // Set ROM : TODO
         break;
      case 3:
      {
         test = true;
         auto fn = [](Emulation* emulation, DataContainer* dnd_container) { emulation->LoadDisk(dnd_container, 0); };
         popup_associated_function_ = std::bind(fn, emulation_, dnd_container);
         AskForSaving(0);
         /*if (!AskForSaving(0))
         {
            emulation_->LoadDisk(dnd_container, 0);
         }*/
         break;
         // Tape - TODO
      }
      case 4:
         // TODO : Ask for tape saving ?

         // Load first element of the container
         //m_pMachine->LoadTape(m_DragFiles[0]);
      {
         MediaManager mediaMgr(dnd_container);
         std::vector<MediaManager::MediaType> list_of_types;
         list_of_types.push_back(MediaManager::MEDIA_TAPE);
         auto list = dnd_container->GetFileList();


         emulation_->LoadTape(list[0]);
         //UpdateStatusBar();
         break;
      }
      case 5:
         emulation_->LoadSnr(paths[0]);
         break;
      case 6:
         emulation_->LoadBin(paths[0]);
         break;
      case 8:
         emulation_->LoadCpr(paths[0]);
         break;
      }
   }
   
}

void SugarboxApp::KeyboardHandler(int key, int scancode, int action, int mods)
{
   // Handle shortcuts

   // Send scancode to emulation
   //if (action == GLFW_PRESS)
   {
      emulation_->GetKeyboardHandler()->SendScanCode(key, true);
   }

   //if (action == GLFW_RELEASE)
   {
      emulation_->GetKeyboardHandler()->SendScanCode(key, false);
   }
}

bool SugarboxApp::AskForSaving(int drive)
{
   if (emulation_->GetEngine()->IsDiskModified(drive))
   {
      PopupType = POPUP_ASK_SAVE;
      PopupArg = drive;
      return true;
   }
   popup_associated_function_();
   return false;
}

bool SugarboxApp::AskForSavingTape()
{
   // TODO : check tape !
   popup_associated_function_();
   return false;
}


void SugarboxApp::Exit()
{
   
}

bool SugarboxApp::PauseEnabled()
{
   return emulation_->EmulationRun() == false;
}

void SugarboxApp::Pause()
{
   emulation_->Pause( );
}

void SugarboxApp::SetSpeed(int speedlimit)
{
   emulation_->GetEngine()->SetSpeed(speedlimit);
}

void SugarboxApp::HardReset()
{
   emulation_->HardReset();
}

void SugarboxApp::ConfigurationSettings()
{
   // Set the Configuration window as opened
   configuration_settings_ = true;
}

void SugarboxApp::InitFileDialogs()
{
   std::vector<std::string> list_format_ext_str;
   std::vector<FormatType*> format_list = disk_builder_.GetFormatsList(DiskBuilder::WRITE);
   unsigned int buffer_ext_length = 0;
   format_ext_map_.clear();
   for (auto it = format_list.begin(); it != format_list.end(); it++)
   {
      //
      std::string ext = std::string(".") + std::string((*it)->GetFormatExt());
      list_format_ext_str.push_back(ext);
      buffer_ext_length += ext.size() + 1;
      format_ext_map_.insert(std::pair< std::string, const FormatType *> (ext, *it));
   }
   buffer_ext_length += 1;
   delete[]write_disk_extension_;
   write_disk_extension_ = new char[buffer_ext_length];
   memset(write_disk_extension_, 0, buffer_ext_length);
   char* ptr = write_disk_extension_;
   for (auto it2 : list_format_ext_str)
   {
      
      ptr = strcat (ptr, it2.c_str());
      unsigned int size_ext = strlen(ptr);
      ptr[size_ext] = '\0';
      ptr = &ptr[size_ext + 1];
   }
   ptr[0] = '\0';

   format_ext_map_read_.clear();
   list_format_ext_str.clear();
   format_list = disk_builder_.GetFormatsList(DiskBuilder::READ);
   buffer_ext_length = 0;
   for (auto it = format_list.begin(); it != format_list.end(); it++)
   {
      //
      std::string ext = std::string(".") + std::string((*it)->GetFormatExt());
      list_format_ext_str.push_back(ext);
      buffer_ext_length += ext.size() + 1;
      format_ext_map_read_.insert(std::pair< std::string, const FormatType*>(ext, *it));
   }
   buffer_ext_length += 1;
   delete[]load_disk_extension_;
   load_disk_extension_ = new char[buffer_ext_length];
   memset(load_disk_extension_, 0, buffer_ext_length);
   ptr = load_disk_extension_;
   for (auto it2 : list_format_ext_str)
   {
      ptr = strcat(ptr, it2.c_str());
      unsigned int size_ext = strlen(ptr);
      ptr[size_ext] = '\0';
      ptr = &ptr[size_ext + 1];
   }
   ptr[0] = '\0';

   load_tape_extension_ = ".wav\0.cdt\0.csw\0\0";
}

bool SugarboxApp::DiskPresent(int drive)
{
   return emulation_->IsDiskPresent(drive);
}

void SugarboxApp::SaveAs(int drive)
{
   // Todo : Generic types, multilanguage, etc.
   file_dialog_type_ = FD_SAVE_AS;
}

void SugarboxApp::Eject(int drive)
{
   // save ?
   popup_associated_function_ = std::bind (&EmulatorEngine::Eject, emulation_->GetEngine(), drive);
   AskForSaving(drive);
}

void SugarboxApp::Flip(int drive)
{
   emulation_->GetEngine()->FlipDisk(drive);
}

void SugarboxApp::InsertSelectFile(int drive)
{
   //ImGuiFileDialog::Instance()->OpenDialog("SaveAs", "Insert disk...", load_disk_extension_, ".");
   file_dialog_type_ = FD_INSERT;
}

void SugarboxApp::InsertSelectTape()
{
   //ImGuiFileDialog::Instance()->OpenDialog("SaveAs", "Insert tape...", load_tape_extension_, ".");
   file_dialog_type_ = FD_INSERT_TAPE;
}

void SugarboxApp::Insert(int drive)
{
   popup_associated_function_ = std::bind(&SugarboxApp::InsertSelectFile, this, drive);
   AskForSaving(drive);
}

void SugarboxApp::InsertBlankDisk(int drive, IDisk::DiskType type)
{
   emulation_->InsertBlankDisk(drive, type);
}

void SugarboxApp::InsertBlank(int drive, IDisk::DiskType type)
{
   popup_associated_function_ = std::bind(&SugarboxApp::InsertBlankDisk, this, drive, type);
   AskForSaving(drive);

}

void SugarboxApp::TapeRecord()
{
   emulation_->TapeRecord();
}

void SugarboxApp::TapePlay()
{
   emulation_->TapePlay();
}

void SugarboxApp::TapeFastForward()
{
   emulation_->TapeFastForward();
}

void SugarboxApp::TapeRewind()
{
   emulation_->TapeRewind();
}

void SugarboxApp::TapePause()
{
   emulation_->TapePause();
}

void SugarboxApp::TapeStop()
{
   emulation_->TapeStop();
}

void SugarboxApp::TapeInsert()
{
   popup_associated_function_ = std::bind(&SugarboxApp::InsertSelectTape, this);
   AskForSavingTape();

}

void SugarboxApp::TapeSaveAs(Emulation::TapeFormat format)
{
   const char* format_ext;
   switch (format)
   {
   case Emulation::TAPE_WAV:
      format_ext = ".wav";
      break;
   case Emulation::TAPE_CDT_DRB:
   case Emulation::TAPE_CDT_CSW:
      format_ext = ".cdt";
      break;
   case Emulation::TAPE_CSW11:
   case Emulation::TAPE_CSW20:
      format_ext = ".csw";
      break;
   }

   //ImGuiFileDialog::Instance()->OpenDialog("SaveAs", "Save Tape as...", format_ext, ".");
   file_dialog_type_ = FD_SAVE_TAPE_AS;
   format_ = format;
}

bool SugarboxApp::IsQuickSnapAvailable()
{
   return emulation_->GetEngine()->IsQuickSnapAvailable();
}

void SugarboxApp::SnaLoad()
{
   //ImGuiFileDialog::Instance()->OpenDialog("SaveAs", "Load snapshot", ".sna\0", ".");
   file_dialog_type_ = FD_INSERT_SNA;
}

void SugarboxApp::SnaSave()
{
   //ImGuiFileDialog::Instance()->OpenDialog("SaveAs", "Save snapshot", ".sna\0", ".");
   file_dialog_type_ = FD_SAVE_SNA;
}

void SugarboxApp::SnaQuickLoad()
{
   emulation_->QuickLoadsnapshot();
}

void SugarboxApp::SnaQuickSave()
{
   emulation_->QuickSavesnapshot();
}

void SugarboxApp::SnrLoad()
{
   //ImGuiFileDialog::Instance()->OpenDialog("SaveAs", "Load SNR", ".snr\0", ".");
   file_dialog_type_ = FD_LOAD_SNR;
}

void SugarboxApp::SnrRecord()
{
   //ImGuiFileDialog::Instance()->OpenDialog("SaveAs", "Save SNR", ".snr\0", ".");
   file_dialog_type_ = FD_RECORD_SNR;
}

bool SugarboxApp::SnrIsRecording()
{
   return emulation_->GetEngine()->IsSnrRecording();
}

bool SugarboxApp::SnrIsReplaying()
{
   return emulation_->GetEngine()->IsSnrReplaying();
}

void SugarboxApp::SnrStopRecord()
{
   emulation_->GetEngine()->StopRecord();
}

void SugarboxApp::SnrStopPlayback()
{
   emulation_->GetEngine()->StopPlayback();
}

void SugarboxApp::CprLoad()
{
   //ImGuiFileDialog::Instance()->OpenDialog("SaveAs", "Load CPR", ".cpr\0.bin\0", ".");
   file_dialog_type_ = FD_LOAD_CPR;
}

bool SugarboxApp::PlusEnabled()
{
   return emulation_->GetEngine()->IsPLUS();
}

bool SugarboxApp::FdcPresent()
{
   return emulation_->GetEngine()->GetSettings()->FDCPlugged();
}

bool SugarboxApp::TapePresent()
{
   // Default is : a tape is always plugged
   return true;
}

bool SugarboxApp::IsAutoloadEnabled()
{
   return emulation_->IsAutoloadEnabled();
}

void SugarboxApp::ToggleAutoload()
{
   action_list_[IFunctionInterface::FN_AUTOLOAD]->action->toggle();
   emulation_->ToggleAutoload();
}

bool SugarboxApp::IsSomethingInClipboard()
{
   /*const char* clipdata = glfwGetClipboardString(window_);
   return (clipdata != nullptr && strlen( glfwGetClipboardString(window_) ) > 0);*/
   return false;
}

void SugarboxApp::AutoType()
{
   //emulation_->AutoType(glfwGetClipboardString(window_));
}

IFunctionInterface::Action* SugarboxApp::AddAction (IFunctionInterface::FunctionType id, std::function<void()> fn, const char* label_id)
{
   
   Action* new_act = new Action;
   new_act ->action = new QAction(tr(""), this);
   new_act->label_id = label_id;

   connect(new_act->action, &QAction::triggered, this, fn);
   action_list_[id] = new_act;
   return new_act;
}

void SugarboxApp::InitAllActions()
{
   AddAction(IFunctionInterface::FN_EXIT, std::bind(&IFunctionInterface::Exit, this), "L_FILE_EXIT");
   AddAction(IFunctionInterface::FN_AUTOLOAD, std::bind(&IFunctionInterface::ToggleAutoload, this), "L_FN_AUTOLOAD")->action->setCheckable(true);
   AddAction(IFunctionInterface::FN_AUTOTYPE, std::bind(&IFunctionInterface::AutoType, this), "L_FN_AUTOTYPE");
   AddAction(IFunctionInterface::FN_CTRL_ONOFF, std::bind(&IFunctionInterface::HardReset, this), "L_CONTROL_ONOFF");
   AddAction(IFunctionInterface::FN_CTRL_PAUSE, std::bind(&IFunctionInterface::Pause, this), "L_CONTROL_PAUSE");

   AddAction(IFunctionInterface::FN_CTRL_SET_SPEED_10, std::bind(&IFunctionInterface::SetSpeed, this, 10), "L_CONTROL_SPEED_10");
   AddAction(IFunctionInterface::FN_CTRL_SET_SPEED_50, std::bind(&IFunctionInterface::SetSpeed, this, 50), "L_CONTROL_SPEED_50");
   AddAction(IFunctionInterface::FN_CTRL_SET_SPEED_100, std::bind(&IFunctionInterface::SetSpeed, this, 100), "L_CONTROL_SPEED_100");
   AddAction(IFunctionInterface::FN_CTRL_SET_SPEED_150, std::bind(&IFunctionInterface::SetSpeed, this, 150), "L_CONTROL_SPEED_150");
   AddAction(IFunctionInterface::FN_CTRL_SET_SPEED_200, std::bind(&IFunctionInterface::SetSpeed, this, 200), "L_CONTROL_SPEED_200");
   AddAction(IFunctionInterface::FN_CTRL_SET_SPEED_400, std::bind(&IFunctionInterface::SetSpeed, this, 400), "L_CONTROL_SPEED_400");
   AddAction(IFunctionInterface::FN_CTRL_SET_SPEED_VSync, std::bind(&IFunctionInterface::SetSpeed, this, -1), "L_CONTROL_SPEED_VSYNC");
   AddAction(IFunctionInterface::FN_CTRL_SET_SPEED_MAX, std::bind(&IFunctionInterface::SetSpeed, this, 0), "L_CONTROL_SPEED_MAX");

   AddAction(IFunctionInterface::FN_CONFIG_SETTINGS, std::bind(&IFunctionInterface::ConfigurationSettings, this), "L_SETTINGS_CONFIG");

   AddAction(IFunctionInterface::FN_DISK_1_SAVE_AS, std::bind(&IFunctionInterface::SaveAs, this, 0), "L_FN_DISK_1_SAVE_AS");
   AddAction(IFunctionInterface::FN_DISK_1_EJECT, std::bind(&IFunctionInterface::Eject, this, 0), "L_FN_DISK_1_EJECT");
   AddAction(IFunctionInterface::FN_DISK_1_FLIP, std::bind(&IFunctionInterface::Flip, this, 0), "L_FN_DISK_1_FLIP");
   AddAction(IFunctionInterface::FN_DISK_1_INSERT, std::bind(&IFunctionInterface::Insert, this, 0), "L_FN_DISK_1_INSERT");
   AddAction(IFunctionInterface::FN_DISK_1_INSERT_BLANK_VENDOR, std::bind(&IFunctionInterface::InsertBlank, this, 0, IDisk::VENDOR), "L_FN_DISK_1_INSERT_BLANK_VENDOR");
   AddAction(IFunctionInterface::FN_DISK_1_INSERT_BLANK_DATA, std::bind(&IFunctionInterface::InsertBlank, this, 0, IDisk::DATA), "L_FN_DISK_1_INSERT_BLANK_DATA");
   AddAction(IFunctionInterface::FN_DISK_2_SAVE_AS, std::bind(&IFunctionInterface::SaveAs, this, 1), "L_FN_DISK_2_SAVE_AS");
   AddAction(IFunctionInterface::FN_DISK_2_EJECT, std::bind(&IFunctionInterface::Eject, this, 1), "L_FN_DISK_2_EJECT");
   AddAction(IFunctionInterface::FN_DISK_2_FLIP, std::bind(&IFunctionInterface::Flip, this, 1), "L_FN_DISK_2_FLIP");
   AddAction(IFunctionInterface::FN_DISK_2_INSERT, std::bind(&IFunctionInterface::Insert, this, 1), "L_FN_DISK_2_INSERT");
   AddAction(IFunctionInterface::FN_DISK_2_INSERT_BLANK_VENDOR, std::bind(&IFunctionInterface::InsertBlank, this, 1, IDisk::VENDOR), "L_FN_DISK_2_INSERT_BLANK_VENDOR");
   AddAction(IFunctionInterface::FN_DISK_2_INSERT_BLANK_DATA, std::bind(&IFunctionInterface::InsertBlank, this, 1, IDisk::DATA), "L_FN_DISK_2_INSERT_BLANK_DATA");

   AddAction(IFunctionInterface::FN_TAPE_RECORD, std::bind(&IFunctionInterface::TapeRecord, this), "L_FN_TAPE_RECORD");
   AddAction(IFunctionInterface::FN_TAPE_PLAY, std::bind(&IFunctionInterface::TapePlay, this), "L_FN_TAPE_PLAY");
   AddAction(IFunctionInterface::FN_TAPE_FASTFORWARD, std::bind(&IFunctionInterface::TapeFastForward, this), "L_FN_TAPE_FASTFORWARD");
   AddAction(IFunctionInterface::FN_TAPE_REWIND, std::bind(&IFunctionInterface::TapeRewind, this), "L_FN_TAPE_REWIND");
   AddAction(IFunctionInterface::FN_TAPE_PAUSE, std::bind(&IFunctionInterface::TapePause, this), "L_FN_TAPE_PAUSE");
   AddAction(IFunctionInterface::FN_TAPE_STOP, std::bind(&IFunctionInterface::TapeStop, this), "L_FN_TAPE_STOP");
   AddAction(IFunctionInterface::FN_TAPE_INSERT, std::bind(&IFunctionInterface::TapeInsert, this), "L_FN_TAPE_INSERT");
   AddAction(IFunctionInterface::FN_TAPE_SAVE_AS_WAV, std::bind(&IFunctionInterface::TapeSaveAs, this, Emulation::TAPE_WAV), "L_FN_TAPE_SAVE_AS_WAV");
   AddAction(IFunctionInterface::FN_TAPE_SAVE_AS_CDT_DRB, std::bind(&IFunctionInterface::TapeSaveAs, this, Emulation::TAPE_CDT_DRB), "L_FN_TAPE_SAVE_AS_CDT_DRB");
   AddAction(IFunctionInterface::FN_TAPE_SAVE_AS_CDT_CSW, std::bind(&IFunctionInterface::TapeSaveAs, this, Emulation::TAPE_CDT_CSW), "L_FN_TAPE_SAVE_AS_CDT_CSW");
   AddAction(IFunctionInterface::FN_TAPE_SAVE_AS_CSW11, std::bind(&IFunctionInterface::TapeSaveAs, this, Emulation::TAPE_CSW11), "L_FN_TAPE_SAVE_AS_CSW11");
   AddAction(IFunctionInterface::FN_TAPE_SAVE_AS_CSW20, std::bind(&IFunctionInterface::TapeSaveAs, this, Emulation::TAPE_CSW20), "L_FN_TAPE_SAVE_AS_CSW20");

   AddAction(IFunctionInterface::FN_SNA_LOAD, std::bind(&IFunctionInterface::SnaLoad, this), "L_FN_LOAD_SNA");
   AddAction(IFunctionInterface::FN_SNA_QUICK_LOAD, std::bind(&IFunctionInterface::SnaQuickLoad, this), "L_FN_QUICK_LOAD_SNA");
   AddAction(IFunctionInterface::FN_SNA_SAVE, std::bind(&IFunctionInterface::SnaSave, this), "L_FN_SAVE_SNA");
   AddAction(IFunctionInterface::FN_SNA_QUICK_SAVE, std::bind(&IFunctionInterface::SnaQuickSave, this), "L_FN_QUICK_SAVE_SNA");
   AddAction(IFunctionInterface::FN_SNR_LOAD, std::bind(&IFunctionInterface::SnrLoad, this), "L_FN_LOAD_SNR");
   AddAction(IFunctionInterface::FN_SNR_RECORD, std::bind(&IFunctionInterface::SnrRecord, this), "L_FN_RECORD_SNR");
   AddAction(IFunctionInterface::FN_SNR_STOP_PLAYING, std::bind(&IFunctionInterface::SnrStopPlayback, this), "L_FN_STOP_PLAYBACK_SNR");
   AddAction(IFunctionInterface::FN_SNR_STOP_RECORD, std::bind(&IFunctionInterface::SnrStopRecord, this), "L_FN_STOP_RECORD_SNR");

   AddAction(IFunctionInterface::FN_CPR_LOAD, std::bind(&IFunctionInterface::CprLoad, this), "L_FN_CPR_LOAD");
}

QAction* SugarboxApp::GiveAction(FunctionType func_type)
{
   // Get action
   QAction* action = action_list_[func_type]->action;
   return action;
}

IFunctionInterface::Action* SugarboxApp::GetFirstAction(FunctionType& func_type)
{
   it_ = action_list_.begin();
   if (it_ != action_list_.end())
   {
      func_type = it_->first;
      return it_->second;
   }
   return nullptr;
}

IFunctionInterface::Action* SugarboxApp::GetNextAction(FunctionType& func_type)
{
   ++it_;
   if (it_ != action_list_.end())
   {
      func_type = it_->first;
      return it_->second;
   }
   return nullptr;
}
