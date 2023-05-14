#include "SugarboxApp.h"
#include "ui_SugarboxApp.h"

#include "SettingsValues.h"

#include <filesystem>
#include <QMouseEvent>

/////////////////////////////////////
// SugarbonApp

SugarboxApp::SugarboxApp(QWidget *parent) : QMainWindow(parent), counter_(0), str_speed_("0%"), 
save_disk_extension_(""), keyboard_handler_(nullptr), language_(), functions_list_(&language_),
dlg_settings_(&config_manager_, this), sound_control_(&sound_mixer_, &language_), debugger_link_(nullptr), debug_(this)
{
   emulation_ = new Emulation(this);

   emulation_->AddNotifierDbg(this);

   connect(&display_, &CDisplay::FrameIsReady, &display_, &CDisplay::Display);

   setWindowTitle(tr("SugarboxV2"));

   setMinimumSize(160, 160);
   resize(800, 600);

   setAcceptDrops(true);
   //setAutoFillBackground(true);
   menuBar()->setFocusPolicy(Qt::ClickFocus);
   setCentralWidget(&display_);
   clear();

   // Create debug window
}

SugarboxApp::~SugarboxApp()
{
   //key_mgr_out.CloseFile();
   delete emulation_;
}

//////////////////////////////////////////////
// QT functions
void SugarboxApp::CreateActions()
{
   QToolBar *fileToolBar = addToolBar(tr("ActionBar"));
   fileToolBar->addAction(action_list_[FN_CTRL_ONOFF]->action);
   QComboBox *config_box = new QComboBox(fileToolBar);
   config_box->setFocusPolicy(Qt::ClickFocus);

   // Create Configuration list
   dlg_settings_.UpdateCombo(config_box);

   fileToolBar->addWidget(config_box);
}

void SugarboxApp::CreateStatusBar()
{
   
}


void SugarboxApp::createMenus ()
{
   for (unsigned int menu_index = 0; menu_index < functions_list_.NbMenu(); menu_index++)
   {
      if (functions_list_.GetMenu(menu_index)->IsAvailable())
      {
         QMenu* menu = menuBar()->addMenu(tr(functions_list_.GetMenu(menu_index)->GetLabel()));
         CreateSubMenu(menu, functions_list_.GetMenu(menu_index), true);
      }
   }
}

void SugarboxApp::CreateSubMenu(QMenu* menu, Function* function, bool toplevel)
{
   if (function->IsNode())
   {
      QMenu* submenu;
      if (function->IsAvailable())
      {
         submenu = (toplevel)?menu: menu->addMenu(tr(function->GetLabel()));
         for (unsigned int submenu_index = 0; submenu_index < function->NbSubMenu(); submenu_index++)
         {
            Function* subfunction = function->GetMenu(submenu_index);
            CreateSubMenu(submenu, function->GetMenu(submenu_index));
         }
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
   return "OpenAL-Soft sound mixer";
}

const char* SugarboxApp::GetFirstSoundName()
{
   return "OpenAL-Soft sound mixer";
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
   UpdateMenu();
   InitFileDialogs();

   // Connect clipboard change to this
   connect(QApplication::clipboard(), SIGNAL(dataChanged()), this, SLOT(UpdateMenu()));
   // Connect menu update
   connect(this, SIGNAL(MenuChanged()), this, SLOT(UpdateMenu()));
}

void SugarboxApp::InitSettings()
{
   // Init standard settings.
   settings_.AddColor(SettingsValues::BACK_COLOR, Qt::white);
   settings_.AddColor(SettingsValues::MARGIN_COLOR, QColor(220, 220, 220));
   settings_.AddColor(SettingsValues::ADDRESS_COLOR,Qt::blue);
   settings_.AddColor(SettingsValues::MNEMONIC_COLOR,Qt::darkBlue);
   settings_.AddColor(SettingsValues::ARGUMENT_COLOR,Qt::darkMagenta);
   settings_.AddColor(SettingsValues::BYTE_COLOR,Qt::darkGray);
   settings_.AddColor(SettingsValues::CHAR_COLOR,Qt::gray);
   settings_.AddColor(SettingsValues::SELECTION_COLOR, QColor(128, 128, 255, 128));

   //
   settings_.AddAction(SettingsValues::DBG_RUN_ACTION, { "DBG_RUN", Qt::Key_F5 });
   settings_.AddAction(SettingsValues::DBG_BREAK_ACTION, { "DBG_BREAK", Qt::Key_F5 });
   settings_.AddAction(SettingsValues::DBG_STEP_ACTION, { "DBG_STEP", Qt::Key_F10 });
   settings_.AddAction(SettingsValues::DBG_STEPIN_ACTION, { "DBG_STEP_IN", Qt::Key_F11 });
   settings_.AddAction(SettingsValues::DBG_STEPOUT_ACTION, { "DBG_STEP_OUT", Qt::Key_F11, Qt::ShiftModifier });
   
   settings_.AddAction(SettingsValues::DBG_TOGGLE_BREAKPOINT_ACTION, { "TGL_BKP", Qt::Key_F9 });
   settings_.AddAction(SettingsValues::DBG_TOGGLE_FLAG_ACTION, { "TGL_FLAG", Qt::Key_F2 });
   
   // Load saved settings
   // todo


   debug_.SetSettings(&settings_);
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
   debug_.SetEmulator(emulation_, &language_);
   debug_.SetFlagHandler(&flag_handler_);

   // Settings
   InitSettings();

   debugger_link_ = new DebugSocket(this, emulation_);
   debugger_link_->StartServer();

   dlg_settings_.Init(emulation_->GetEngine());
   keyboard_handler_ = emulation_->GetKeyboardHandler();

   // Get current directory, and add the CONF to it
   current_path_exe /= "CONF";
   dlg_settings_.Refresh(current_path_exe.string().c_str());

   InitMenu();
   createMenus();
   CreateActions();
   CreateStatusBar();



   // This part was used to convert keyboard from windows to keycode (for cross platform usage)
   // It's no longer used, but I think I don't want to lose this code :)
   // Set a map to convert scancode to key
   /*
   std::filesystem::path key_path = current_path_exe;
   key_path /= "KeyboardMaps.ini";
   key_mgr.OpenFile(key_path.string().c_str());

   std::filesystem::path key_path_out = current_path_exe;
   key_path_out /= "KeyboardMaps_Generic.ini";
   key_mgr_out.OpenFile(key_path_out.string().c_str());
   */
   // Run main loop

   return 0;
}

void SugarboxApp::keyPressEvent(QKeyEvent * event_keyboard)
{
   // Convert this key to new config file !
   keyboard_handler_->SendScanCode(event_keyboard->key(), true);
   event_keyboard->ignore();
   /*keyboard_handler_->SendScanCode(event_keyboard->nativeScanCode(), true);

   int value_key = event_keyboard->key();

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
            if (value_sc == event_keyboard->nativeScanCode())
            {
               char base_key[32];
               memset(base_key, 0, sizeof(base_key));
               strncpy(base_key, key, 4);

               //key_mgr.GetConfiguration(section, base_key, "", char_buffer, 16);

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

                  // SCA, char, uchar, charctrl, ucharctrl
                  strcpy(&base_key[4], "SCA");
                  key_mgr.GetConfiguration(section, base_key, "", char_buffer, 16);
                  key_mgr_out.SetConfiguration(section, base_key, char_buffer);

                  strcpy(&base_key[4], "char");
                  key_mgr.GetConfiguration(section, base_key, "", char_buffer, 16);
                  key_mgr_out.SetConfiguration(section, base_key, char_buffer);

                  strcpy(&base_key[4], "char_value");
                  key_mgr.GetConfiguration(section, base_key, "", char_buffer, 16);
                  key_mgr_out.SetConfiguration(section, base_key, char_buffer);

                  strcpy(&base_key[4], "char_Alt");
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

         }

         key = key_mgr.GetNextKey();
      }
      section = key_mgr.GetNextSection();
   }
   //key_mgr_out.CloseFile();
   */
}

void SugarboxApp::keyReleaseEvent(QKeyEvent *event_keyboard)
{
   keyboard_handler_->SendScanCode(event_keyboard->key(), false);
}

void SugarboxApp::DrawStatusBar()
{
   // Speed of emulation : only 1 every 50 frames
   counter_++;

   if (counter_ == 50 /*|| m_pMachine->GetMonitor()->m_bSpeed*/)
   {
      // Update

      std::snprintf(str_speed_, sizeof(str_speed_), "%i %%%%", emulation_->GetSpeed());
      counter_ = 0;
      statusBar()->showMessage(tr(str_speed_), 2000);
   }
   sound_control_.DrawSoundVolume();
}

void SugarboxApp::SizeChanged(int width, int height)
{
   //glViewport(0, height - toolbar_height - main_display_height, main_display_width, main_display_height);
}

bool SugarboxApp::AskForSaving(int drive)
{
   if (emulation_->GetEngine()->IsDiskModified(drive))
   {
      QMessageBox msgBox;
      msgBox.setText(tr(language_.GetString("L_FILEDIALOG_DISK_CHANGED")));
      msgBox.setInformativeText(tr(language_.GetString("L_FILEDIALOG_CHANGED_QUESTION")));
      msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
      msgBox.setDefaultButton(QMessageBox::Save);
      int ret = msgBox.exec();
      switch (ret)
      {
      case QMessageBox::Save:

         return true;
         break;
      case QMessageBox::Discard:
         return true;
         break;
      case QMessageBox::Cancel:
         return false;
         break;
      default:
         // should never be reached
         break;
      }
      return true;
   }
   return true;
}

bool SugarboxApp::AskForSavingTape()
{
   if (emulation_->GetEngine()->IsTapeChanged())
   {
      QMessageBox msgBox;
      msgBox.setText(tr(language_.GetString("L_FILEDIALOG_TAPE_CHANGED")));
      msgBox.setInformativeText(tr(language_.GetString("L_FILEDIALOG_CHANGED_QUESTION")));
      msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
      msgBox.setDefaultButton(QMessageBox::Save);
      int ret = msgBox.exec();
      switch (ret)
      {
      case QMessageBox::Save:

         return true;
         break;
      case QMessageBox::Discard:
         return true;
         break;
      case QMessageBox::Cancel:
         return false;
         break;
      default:
         // should never be reached
         break;
      }
      return true;
   }
   return true;
}

bool SugarboxApp::PauseEnabled()
{
   return emulation_->EmulationRun() == false;
}

void SugarboxApp::Pause()
{
   emulation_->Pause( );
   UpdateMenu();
}

bool SugarboxApp::CheckSpeed(int speedlimit)
{
   return emulation_->GetEngine()->GetSpeedLimit() == speedlimit;
}

void SugarboxApp::SetSpeed(int speedlimit)
{
   emulation_->GetEngine()->SetSpeed(speedlimit);
   UpdateMenu();
}

void SugarboxApp::ConfigurationSettings()
{
}


void SugarboxApp::OpenDebugger()
{
   // Open debugger windows
   debug_.Update();
   debug_.show();
}

void SugarboxApp::InitFileDialogs()
{
   std::vector<FormatType*> format_list = disk_builder_.GetFormatsList(DiskBuilder::WRITE);
   save_disk_extension_.clear();
   format_ext_map_.clear();
   for (auto it = format_list.begin(); it != format_list.end(); it++)
   {
      //
      QString ext = QString(".") + QString((*it)->GetFormatExt());
      format_ext_map_.insert(std::pair< QString, const FormatType *> (ext, *it));

      if (it != format_list.begin())
      {
         save_disk_extension_ += ";;";
      }
      save_disk_extension_ += QString((*it)->GetFormatDescriptor()) + QString(" (*.") + QString((*it)->GetFormatExt()) + QString(")");
   }

   format_ext_map_read_.clear();
   format_list = disk_builder_.GetFormatsList(DiskBuilder::READ);
   
   load_disk_extension_ = "Disk dump (";
   for (auto it = format_list.begin(); it != format_list.end(); it++)
   {
      //
      if (it != format_list.begin())
      {
         load_disk_extension_ += " ";
      }
         
      load_disk_extension_ += QString("*.") + QString((*it)->GetFormatExt());

      QString ext = QString((*it)->GetFormatDescriptor()) + QString(" (*.") + QString((*it)->GetFormatExt()) + QString(")");
      format_ext_map_read_.insert(std::pair< QString, const FormatType*>(ext, *it));
   }
   load_disk_extension_ += ")";

   // Tape
   load_tape_extension_ = "Tape dump (*.wav *.cdt *.csw *.tzx)";
   save_tape_extension_ = "WAV (*.wav);;CDT-drb (*.cdt);;CDT-CSW (*.cdt);;CSW 1.1 (*.csw);;CSW 2.0 (*.csw)";
   format_tape_save_["WAV (*.wav)"] = Emulation::TAPE_WAV;
   format_tape_save_["CDT-drb (*.cdt)"] = Emulation::TAPE_CDT_DRB;
   format_tape_save_["CDT-CSW (*.cdt)"] = Emulation::TAPE_CDT_CSW;
   format_tape_save_["CSW 1.1 (*.csw)"] = Emulation::TAPE_CSW11;
   format_tape_save_["CSW 2.0 (*.csw)"] = Emulation::TAPE_CSW20;
}

void SugarboxApp::UpdateMenu()
{
   functions_list_.UpdateStatus();
}

void SugarboxApp::SaveAs(int drive)
{
   QFileDialog fd (this);
   fd.setFileMode(QFileDialog::AnyFile);
   fd.setAcceptMode(QFileDialog::AcceptSave);
   fd.setNameFilter(save_disk_extension_);
   fd.setViewMode(QFileDialog::Detail);
   if (fd.exec())
   {
      QStringList fileNames = fd.selectedFiles();
      QString ext_seletected = fd.selectedNameFilter();
      emulation_->SaveDiskAs(drive, fileNames[0].toUtf8(), format_ext_map_[ext_seletected]);
   }
}

void SugarboxApp::Eject(int drive)
{
   if (AskForSaving(drive))
   {
      emulation_->GetEngine()->Eject(drive);
   }
}

void SugarboxApp::Insert(int drive)
{
   if (AskForSaving(drive))
   {
      QString str = QFileDialog::getOpenFileName(this, tr(language_.GetString("L_FILEDIALOG_INSERT_DISK")), "", load_disk_extension_);
      if (str.size() > 0)
      {
         emulation_->LoadDisk(str.toUtf8(), drive);
      }
   }
}

void SugarboxApp::InsertBlank(int drive, IDisk::DiskType type)
{
   if ( AskForSaving(drive))
   {
      emulation_->InsertBlankDisk(drive, type);
   }
}

void SugarboxApp::FullScreenToggle ()
{
   static bool fs = false;

   if (fs)
   {
      showNormal();
   }
   else
   {
      // Toggle fullscreen
      setWindowState(Qt::WindowFullScreen);
      showFullScreen();
   }
   fs = !fs;
}

void SugarboxApp::TapeInsert()
{
   if ( AskForSavingTape())
   {
      QString str = QFileDialog::getOpenFileName(this, tr(language_.GetString("L_FILEDIALOG_INSERT_TAPE")), "", load_tape_extension_);
      if (str.size() > 0)
      {
         emulation_->LoadTape(str.toUtf8());
      }
   }
}

void SugarboxApp::TapeSaveAs()
{
   QFileDialog fd(this);
   fd.setFileMode(QFileDialog::AnyFile);
   fd.setAcceptMode(QFileDialog::AcceptSave);
   fd.setNameFilter(save_tape_extension_);
   fd.setViewMode(QFileDialog::Detail);
   if (fd.exec())
   {
      QStringList fileNames = fd.selectedFiles();
      QString ext_seletected = fd.selectedNameFilter();
      emulation_->SaveTapeAs(fileNames[0].toUtf8(), format_tape_save_[ext_seletected]);
   }
}

void SugarboxApp::SnaLoad()
{
   QString str = QFileDialog::getOpenFileName(this, tr(language_.GetString("L_FN_LOAD_SNA")), "", "Snapshot file (*.SNA *.sna)");
   if (str.size() > 0)
   {
      emulation_->LoadSnapshot(str.toUtf8());
   }
}

void SugarboxApp::SnaSave()
{
   QString str = QFileDialog::getSaveFileName(this, tr(language_.GetString("L_FN_LOAD_SNA")), "", "Snapshot file (*.SNA *.sna)");
   if (str.size() > 0)
   {
      emulation_->SaveSnapshot(str.toUtf8());
   }
}

void SugarboxApp::SnrLoad()
{
   QString str = QFileDialog::getOpenFileName(this, tr(language_.GetString("L_FN_LOAD_SNR")), "", "Snapshot Record file (*.SNR *.snr)");
   if (str.size() > 0)
   {
      emulation_->LoadSnr(str.toUtf8());
   }
}

void SugarboxApp::SnrRecord()
{
   QString str = QFileDialog::getSaveFileName(this, tr(language_.GetString("L_FN_LOAD_SNR")), "", "Snapshot Record file (*.SNR *.snr)");
   if (str.size() > 0)
   {
      emulation_->RecordSnr(str.toUtf8());
   }
}

void SugarboxApp::CprLoad()
{
   QString str = QFileDialog::getOpenFileName(this, tr(language_.GetString("L_FN_CPR_LOAD")), "", "Cartridge file (*.bin *.cpr)");
   if (str.size() > 0)
   {
      emulation_->LoadCpr(str.toUtf8());
   }
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

void SugarboxApp::ToggleAutoload()
{
   emulation_->ToggleAutoload();
   UpdateMenu();
}

bool SugarboxApp::IsSomethingInClipboard()
{
   const QClipboard *clipboard = QApplication::clipboard();
   return (clipboard->text().size() > 0);
}

void SugarboxApp::AutoType()
{
   const QClipboard *clipboard = QApplication::clipboard();
   emulation_->AutoType(clipboard->text().toUtf8());   
}

IFunctionInterface::Action* SugarboxApp::AddAction (IFunctionInterface::FunctionType id, std::function<void()> fn, const char* label_id, std::function<bool()>enabled, std::function<bool()>checked)
{

   Action* new_act = new Action;
   new_act ->action = new QAction(tr(""), this);
   new_act->label_id = label_id;
   new_act->checked = checked;
   new_act->enabled = enabled;
   if ( checked != nullptr)
      new_act->action->setCheckable(true);
   if (enabled != nullptr)
      new_act->action->setEnabled(true);

   connect(new_act->action, &QAction::triggered, this, fn);
   action_list_[id] = new_act;

   return new_act;
}

void SugarboxApp::InitAllActions()
{
   Action* act;
   AddAction(IFunctionInterface::FN_EXIT, std::bind(&SugarboxApp::close, this), "L_FILE_EXIT");
   AddAction(IFunctionInterface::FN_AUTOLOAD, std::bind(&SugarboxApp::ToggleAutoload, this), "L_FN_AUTOLOAD", nullptr, std::bind(&Emulation::IsAutoloadEnabled, emulation_));
   AddAction(IFunctionInterface::FN_AUTOTYPE, std::bind(&SugarboxApp::AutoType, this), "L_FN_AUTOTYPE", std::bind(&SugarboxApp::IsSomethingInClipboard, this));
   AddAction(IFunctionInterface::FN_CTRL_ONOFF, std::bind(&Emulation::HardReset, emulation_), "L_CONTROL_ONOFF");
   AddAction(IFunctionInterface::FN_CTRL_PAUSE, std::bind(&SugarboxApp::Pause, this), "L_CONTROL_PAUSE", nullptr, std::bind(&SugarboxApp::PauseEnabled, this));

   AddAction(IFunctionInterface::FN_CTRL_SET_SPEED_10, std::bind(&SugarboxApp::SetSpeed, this, 10), "L_CONTROL_SPEED_10", nullptr, std::bind(&SugarboxApp::CheckSpeed, this, 10) );
   AddAction(IFunctionInterface::FN_CTRL_SET_SPEED_50, std::bind(&SugarboxApp::SetSpeed, this, 50), "L_CONTROL_SPEED_50", nullptr, std::bind(&SugarboxApp::CheckSpeed, this, 50));
   AddAction(IFunctionInterface::FN_CTRL_SET_SPEED_100, std::bind(&SugarboxApp::SetSpeed, this, 100), "L_CONTROL_SPEED_100", nullptr, std::bind(&SugarboxApp::CheckSpeed, this, 100));
   AddAction(IFunctionInterface::FN_CTRL_SET_SPEED_150, std::bind(&SugarboxApp::SetSpeed, this, 150), "L_CONTROL_SPEED_150", nullptr, std::bind(&SugarboxApp::CheckSpeed, this, 150));
   AddAction(IFunctionInterface::FN_CTRL_SET_SPEED_200, std::bind(&SugarboxApp::SetSpeed, this, 200), "L_CONTROL_SPEED_200", nullptr, std::bind(&SugarboxApp::CheckSpeed, this, 200));
   AddAction(IFunctionInterface::FN_CTRL_SET_SPEED_400, std::bind(&SugarboxApp::SetSpeed, this, 400), "L_CONTROL_SPEED_400", nullptr, std::bind(&SugarboxApp::CheckSpeed, this, 400));
   AddAction(IFunctionInterface::FN_CTRL_SET_SPEED_VSync, std::bind(&SugarboxApp::SetSpeed, this, -1), "L_CONTROL_SPEED_VSYNC", nullptr, std::bind(&SugarboxApp::CheckSpeed, this, -1));
   AddAction(IFunctionInterface::FN_CTRL_SET_SPEED_MAX, std::bind(&SugarboxApp::SetSpeed, this, 0), "L_CONTROL_SPEED_MAX", nullptr, std::bind(&SugarboxApp::CheckSpeed, this, 0));

   AddAction(IFunctionInterface::FN_CONFIG_SETTINGS, std::bind(&SugarboxApp::ConfigurationSettings, this), "L_SETTINGS_CONFIG");

   AddAction(IFunctionInterface::FN_DEBUG_DEBUGGER, std::bind(&SugarboxApp::OpenDebugger, this), "L_DEBUG_OPEN_DBG");

   AddAction(IFunctionInterface::FN_DISK_1_SAVE_AS, std::bind(&SugarboxApp::SaveAs, this, 0), "L_FN_DISK_1_SAVE_AS", std::bind(&Emulation::IsDiskPresent, emulation_, 0));
   AddAction(IFunctionInterface::FN_DISK_1_EJECT, std::bind(&SugarboxApp::Eject, this, 0), "L_FN_DISK_1_EJECT", std::bind(&Emulation::IsDiskPresent, emulation_, 0));
   AddAction(IFunctionInterface::FN_DISK_1_FLIP, std::bind(&EmulatorEngine::FlipDisk, emulation_->GetEngine(), 0), "L_FN_DISK_1_FLIP", std::bind(&Emulation::IsDiskPresent, emulation_, 0));
   AddAction(IFunctionInterface::FN_DISK_1_INSERT, std::bind(&SugarboxApp::Insert, this, 0), "L_FN_DISK_1_INSERT");
   AddAction(IFunctionInterface::FN_DISK_1_INSERT_BLANK_VENDOR, std::bind(&SugarboxApp::InsertBlank, this, 0, IDisk::VENDOR), "L_FN_DISK_1_INSERT_BLANK_VENDOR");
   AddAction(IFunctionInterface::FN_DISK_1_INSERT_BLANK_DATA, std::bind(&SugarboxApp::InsertBlank, this, 0, IDisk::DATA), "L_FN_DISK_1_INSERT_BLANK_DATA");
   AddAction(IFunctionInterface::FN_DISK_2_SAVE_AS, std::bind(&SugarboxApp::SaveAs, this, 1), "L_FN_DISK_2_SAVE_AS", std::bind(&Emulation::IsDiskPresent, emulation_, 1));
   AddAction(IFunctionInterface::FN_DISK_2_EJECT, std::bind(&SugarboxApp::Eject, this, 1), "L_FN_DISK_2_EJECT", std::bind(&Emulation::IsDiskPresent, emulation_, 1));
   AddAction(IFunctionInterface::FN_DISK_2_FLIP, std::bind(&EmulatorEngine::FlipDisk, emulation_->GetEngine(), 1), "L_FN_DISK_2_FLIP", std::bind(&Emulation::IsDiskPresent, emulation_, 1));
   AddAction(IFunctionInterface::FN_DISK_2_INSERT, std::bind(&SugarboxApp::Insert, this, 1), "L_FN_DISK_2_INSERT");
   AddAction(IFunctionInterface::FN_DISK_2_INSERT_BLANK_VENDOR, std::bind(&SugarboxApp::InsertBlank, this, 1, IDisk::VENDOR), "L_FN_DISK_2_INSERT_BLANK_VENDOR");
   AddAction(IFunctionInterface::FN_DISK_2_INSERT_BLANK_DATA, std::bind(&SugarboxApp::InsertBlank, this, 1, IDisk::DATA), "L_FN_DISK_2_INSERT_BLANK_DATA");

   AddAction(IFunctionInterface::FN_TAPE_RECORD, std::bind(&Emulation::TapeRecord, emulation_), "L_FN_TAPE_RECORD");
   AddAction(IFunctionInterface::FN_TAPE_PLAY, std::bind(&Emulation::TapePlay, emulation_), "L_FN_TAPE_PLAY");
   AddAction(IFunctionInterface::FN_TAPE_FASTFORWARD, std::bind(&Emulation::TapeFastForward, emulation_), "L_FN_TAPE_FASTFORWARD");
   AddAction(IFunctionInterface::FN_TAPE_REWIND, std::bind(&Emulation::TapeRewind, emulation_), "L_FN_TAPE_REWIND");
   AddAction(IFunctionInterface::FN_TAPE_PAUSE, std::bind(&Emulation::TapePause, emulation_), "L_FN_TAPE_PAUSE");
   AddAction(IFunctionInterface::FN_TAPE_STOP, std::bind(&Emulation::TapeStop, emulation_), "L_FN_TAPE_STOP");
   AddAction(IFunctionInterface::FN_TAPE_INSERT, std::bind(&SugarboxApp::TapeInsert, this), "L_FN_TAPE_INSERT");
   AddAction(IFunctionInterface::FN_TAPE_SAVE_AS, std::bind(&SugarboxApp::TapeSaveAs, this), "L_FN_MENU_SAVE_TAPE_AS");

   AddAction(IFunctionInterface::FN_SNA_LOAD, std::bind(&SugarboxApp::SnaLoad, this), "L_FN_LOAD_SNA");
   AddAction(IFunctionInterface::FN_SNA_QUICK_LOAD, std::bind(&Emulation::QuickLoadsnapshot, emulation_), "L_FN_QUICK_LOAD_SNA", std::bind(&EmulatorEngine::IsQuickSnapAvailable, emulation_->GetEngine()));
   AddAction(IFunctionInterface::FN_SNA_SAVE, std::bind(&SugarboxApp::SnaSave, this), "L_FN_SAVE_SNA");
   AddAction(IFunctionInterface::FN_SNA_QUICK_SAVE, std::bind(&Emulation::QuickSavesnapshot, emulation_), "L_FN_QUICK_SAVE_SNA", std::bind(&EmulatorEngine::IsQuickSnapAvailable, emulation_->GetEngine()));
   AddAction(IFunctionInterface::FN_SNR_LOAD, std::bind(&SugarboxApp::SnrLoad, this), "L_FN_LOAD_SNR");
   AddAction(IFunctionInterface::FN_SNR_RECORD, std::bind(&SugarboxApp::SnrRecord, this), "L_FN_RECORD_SNR");
   AddAction(IFunctionInterface::FN_SNR_STOP_PLAYING, std::bind(&EmulatorEngine::StopPlayback, emulation_->GetEngine()), "L_FN_STOP_PLAYBACK_SNR", std::bind(&EmulatorEngine::IsSnrReplaying, emulation_->GetEngine()));
   AddAction(IFunctionInterface::FN_SNR_STOP_RECORD, std::bind(&EmulatorEngine::StopRecord, emulation_->GetEngine()), "L_FN_STOP_RECORD_SNR", std::bind(&EmulatorEngine::IsSnrRecording, emulation_->GetEngine()));

   AddAction(IFunctionInterface::FN_CPR_LOAD, std::bind(&SugarboxApp::CprLoad, this), "L_FN_CPR_LOAD");

   // Display
   act = AddAction(IFunctionInterface::FN_DIS_FULLSCREEN, std::bind(&SugarboxApp::FullScreenToggle, this), "L_FN_CPR_LOAD");

   act->action->setShortcut(Qt::Key_F1);
   addAction(act->action);
}

QAction* SugarboxApp::GetAction(FunctionType func_type)
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

void SugarboxApp::clear()
{
   setBackgroundRole(QPalette::Dark);
   emit changed();
}

void SugarboxApp::dragEnterEvent(QDragEnterEvent *event)
{
   if (event->mimeData()->hasUrls())
   {
      setBackgroundRole(QPalette::Highlight);
      event->acceptProposedAction();
      emit changed(event->mimeData());
   }
}

void SugarboxApp::dragMoveEvent(QDragMoveEvent *event)
{
   event->acceptProposedAction();
}

void SugarboxApp::dropEvent(QDropEvent *event)
{
   foreach(const QUrl &url, event->mimeData()->urls())
   {
      QString fileName = url.toLocalFile();
      std::string path = fileName.toUtf8().constData();
      
      DataContainer* dnd_container = emulation_->CanLoad(path.c_str());

      if (dnd_container == nullptr)
         continue;
      MediaManager mediaMgr(dnd_container);
      std::vector<MediaManager::MediaType> list_of_types;
      list_of_types.push_back(MediaManager::MEDIA_DISK);
      list_of_types.push_back(MediaManager::MEDIA_SNA);
      list_of_types.push_back(MediaManager::MEDIA_SNR);
      list_of_types.push_back(MediaManager::MEDIA_TAPE);
      list_of_types.push_back(MediaManager::MEDIA_BIN);
      list_of_types.push_back(MediaManager::MEDIA_CPR);
      list_of_types.push_back(MediaManager::MEDIA_XPR);

      int media_type = mediaMgr.GetType(list_of_types);

      switch (media_type)
      {
         // Test : Is it SNA?
      case 1:
         emulation_->LoadSnapshot(path.c_str());
         break;
      case 2:
         // Set ROM : TODO
         break;
      case 3:
      {
         if (AskForSaving(0))
         {
            emulation_->LoadDisk(dnd_container, 0);
         }
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
         emulation_->LoadSnr(path.c_str());
         break;
      case 6:
         emulation_->LoadBin(path.c_str());
         break;
      case 8:
         emulation_->LoadCpr(path.c_str());
         break;
      case MediaManager::MEDIA_XPR:
         emulation_->LoadXpr(path.c_str());
         break;
      }
   }
}

void SugarboxApp::dragLeaveEvent(QDragLeaveEvent *event)
{
   clear();
   event->accept();
}

void SugarboxApp::DiskLoaded()
{
   // Signal an update 
   emit MenuChanged();
}

void SugarboxApp::ChangeSettings(MachineSettings* settings)
{
   settings->Load();
   emulation_->ChangeConfig(settings);
   // Set the keyboiard focus to display again (not combo)
   display_.setFocus();

}

void SugarboxApp::NotifyStop()
{
   // call update for debugger
   debug_.Update();
}