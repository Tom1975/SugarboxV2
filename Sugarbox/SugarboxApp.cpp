#include "SugarboxApp.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "ImGuiFileDialog/ImGuiFileDialog.h"

#include <GL/gl3w.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <filesystem>

/////////////////////////////////////
// Generic callbacks
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
   SugarboxApp* app = (SugarboxApp*)glfwGetWindowUserPointer(window);
   app->SizeChanged(width, height);
}

void cursor_enter_callback(GLFWwindow* window, int entered)
{
   // Enter ?
   const char* content_of_clipboard = glfwGetClipboardString(window);
}

void joystick_callback(int jid, int event)
{
   if (event == GLFW_CONNECTED)
   {
      // The joystick was connected
   }
   else if (event == GLFW_DISCONNECTED)
   {
      // The joystick was disconnected
   }
}

void DropCallback(GLFWwindow* window, int count, const char** paths)
{
   SugarboxApp* app = (SugarboxApp*)glfwGetWindowUserPointer(window);
   app->Drop(count, paths);
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
   SugarboxApp* app = (SugarboxApp*)glfwGetWindowUserPointer(window);
   app->KeyboardHandler(key, scancode, action, mods);
}

/////////////////////////////////////
// SugarbonApp

SugarboxApp::SugarboxApp() : counter_(0), str_speed_("0%"), write_disk_extension_(nullptr), load_disk_extension_(nullptr), keyboard_handler_(nullptr), language_(), functions_list_(&language_),
dlg_settings_(&config_manager_), sound_control_(&sound_mixer_, &language_), configuration_settings_(false)
{
  
}

SugarboxApp::~SugarboxApp()
{
   delete[]write_disk_extension_;
   delete[]load_disk_extension_;
}

void error_callback(int error, const char* description)
{
   fprintf(stderr, "Error: %s\n", description);
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
   functions_list_.InitFunctions(this);
   language_.Init("Resources/labels.ini");

   InitFileDialogs();
}

int SugarboxApp::RunApp()
{
   std::filesystem::path current_path_exe = std::filesystem::current_path();

   // Generic init
   InitMenu();

   // GLFW iniut
   if (!glfwInit())
   {
      // Initialization failed
      return -1; 
   }
   glfwSetErrorCallback(error_callback);

   glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
   glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // We want OpenGL 3.3
   glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
   glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
   glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // We don't want the old OpenGL 

   // Create the main window
   window_width_ = main_display_width + peripherals_width;
   window_height_ = main_display_height + toolbar_height + status_height;

   // Window creation
   window_ = glfwCreateWindow(window_width_, window_height_, "Sugarbox", NULL, NULL);

   if (!window_)
   {
      // Window or OpenGL context creation failed
      return -1;
   }
   glfwSetWindowUserPointer(window_, (void*)this);
   glfwMakeContextCurrent(window_);

   glfwSetKeyCallback(window_, KeyCallback);
   glfwSetDropCallback(window_, DropCallback);
   glfwSetJoystickCallback(joystick_callback);
   glfwSetFramebufferSizeCallback(window_, framebuffer_size_callback);
   glfwSetCursorEnterCallback(window_, cursor_enter_callback);

   bool err = gl3wInit() != 0;

   // Init sound
   // TODO
   
   // Init Gui
   ImGui::CreateContext();
   ImGui_ImplGlfw_InitForOpenGL(window_, true);
   const char* glsl_version = "#version 130";
   ImGui_ImplOpenGL3_Init(glsl_version);
   
   // Init display
   SizeChanged(window_width_, window_height_);

   display_.Init();
   emulation_.Init(&display_, this, current_path_exe.string().c_str());
   dlg_settings_.Init(emulation_.GetEngine());
   keyboard_handler_ = emulation_.GetKeyboardHandler();
   
   // Get current directory, and add the CONF to it
   current_path_exe /= "CONF";
   dlg_settings_.Refresh(current_path_exe.string().c_str());

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

            }
         }

         key = key_mgr.GetNextKey();
      }
      section = key_mgr.GetNextSection();
   }
   key_mgr_out.CloseFile();
   */




   // Run main loop
   RunMainLoop();

   glfwDestroyWindow(window_);
   glfwTerminate();

   return 0;
}

static bool test = false;

void SugarboxApp::RunMainLoop()
{
   while (!glfwWindowShouldClose(window_))
   {
      glfwPollEvents();

      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplGlfw_NewFrame();

      glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT);

      DrawMainWindow();

      ImGui::NewFrame();
      DrawMenu();
      DrawPeripherals();
      DrawStatusBar();
      DrawOthers();

      if (ImGuiFileDialog::Instance()->FileDialog("SaveAs"))
      {
         ImGuiFileDialog* imgui_fd = ImGuiFileDialog::Instance();
         switch (file_dialog_type_)
         {
         case FD_SAVE_AS:
            emulation_.SaveDiskAs(0, imgui_fd->GetFilepathName().c_str(), format_ext_map_[imgui_fd->GetCurrentFilter()]);
            break;
         case FD_INSERT:
            emulation_.LoadDisk( imgui_fd->GetFilepathName().c_str(), 0);
            break;
         case FD_INSERT_TAPE:
            emulation_.LoadTape(imgui_fd->GetFilepathName().c_str());
            break;
         case FD_SAVE_TAPE_AS:
            emulation_.SaveTapeAs(imgui_fd->GetFilepathName().c_str(), format_);
            break;
         case FD_INSERT_SNA:
            emulation_.LoadSnapshot(imgui_fd->GetFilepathName().c_str());
         }
         ImGuiFileDialog::Instance()->CloseDialog("SaveAs");
      }

      HandlePopups();

      ImGui::Render();
      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
      
      // Keep running
      glfwSwapBuffers(window_);
   }

   emulation_.Stop();
}

void SugarboxApp::DrawMainWindow()
{
   // Draw texture
   display_.Display();
}

void SugarboxApp::DrawSubMenu(Function* submenu)
{
   if (submenu->IsNode())
   {
      if (ImGui::BeginMenu(submenu->GetLabel()))
      {
         for (int submenu_index = 0; submenu_index < submenu->NbSubMenu(); submenu_index++)
         {
            DrawSubMenu(submenu->GetMenu(submenu_index));
         }
         ImGui::EndMenu();
      }
   }
   else
   {
      if (ImGui::MenuItem(
         submenu->GetLabel(),
         submenu->GetShortcut(),
         submenu->IsSelected(),
         submenu->IsAvailable()))
      {
         submenu->Call();
      }

   }
}

void SugarboxApp::DrawMenu()
{
   ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
   ImGui::SetNextWindowSize(ImVec2(window_width_, toolbar_height), ImGuiCond_Always);

   ImGui::Begin("Toolbar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_MenuBar);
   if (ImGui::BeginMenuBar())
   {
      for (int menu_index = 0; menu_index < functions_list_.NbMenu(); menu_index++)
      {
         DrawSubMenu(functions_list_.GetMenu(menu_index));
      }
      ImGui::EndMenuBar();
   }

   // Toolbar : Add configs
   dlg_settings_.DisplayConfigCombo();

   ImGui::End();
}

void SugarboxApp::DrawPeripherals()
{
   ImGui::SetNextWindowPos(ImVec2(main_display_width, toolbar_height), ImGuiCond_Always);
   ImGui::SetNextWindowSize(ImVec2(peripherals_width, main_display_height), ImGuiCond_Always);
   ImGui::Begin("Peripherals", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_MenuBar);
   // Disk drives
   // Tape

   ImGui::End();
}

void SugarboxApp::DrawStatusBar()
{
   ImGui::SetNextWindowPos(ImVec2(0, window_height_ - status_height), ImGuiCond_Always);
   ImGui::SetNextWindowSize(ImVec2(window_width_, status_height), ImGuiCond_Always);
   ImGui::Begin("Status bar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_MenuBar);

      // Speed of emulation : only 1 every 50 frames
      counter_++;

      if (counter_ == 50 /*|| m_pMachine->GetMonitor()->m_bSpeed*/)
      {
         // Update
      
         sprintf(str_speed_, "%i %%%%", emulation_.GetSpeed());
         counter_ = 0;
      }
      ImGui::Text(str_speed_);

      // Sound control
      ImGui::SameLine();
      sound_control_.DrawSoundVolume();

   ImGui::End();
}

void SugarboxApp::DrawOthers()
{
   // Configuration ?
   
   if (configuration_settings_)
   {
      dlg_settings_.DisplayMenu();
   }
}

void SugarboxApp::HandlePopups()
{
   switch (PopupType)
   {
      case POPUP_ASK_SAVE:
      {
         char buffer[128];
         sprintf(buffer, "Disk %c has changed ! Do you want to save it ?", (PopupArg == 0) ? 'A' : 'B');
         ImGui::OpenPopup(buffer);
         if (ImGui::BeginPopupModal(buffer, NULL)) {
            if (ImGui::Button("Yes"))
            {
               emulation_.GetEngine()->SaveDisk(PopupArg);
               PopupType = POPUP_NONE;
               popup_associated_function_();
            }
            if (ImGui::Button("No"))
            {
               emulation_.GetEngine()->SaveDisk(PopupArg);
               PopupType = POPUP_NONE;
               popup_associated_function_();
            }
            // Call associated function
            ImGui::EndPopup();
         }
         break;
      }
   }

}


void SugarboxApp::SizeChanged(int width, int height)
{
   glViewport(0, height - toolbar_height - main_display_height, main_display_width, main_display_height);
}

void SugarboxApp::Drop(int count, const char** paths)
{
   // Check for headers :
   for (int i = 0; i < 4 && i < count; i++)
   {
      DataContainer* dnd_container = emulation_.CanLoad(paths[i]);

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
         emulation_.LoadSnapshot(paths[0]);
         break;
      case 2:
         // Set ROM : TODO
         break;
      case 3:
      {
         test = true;
         auto fn = [](Emulation* emulation, DataContainer* dnd_container) { emulation->LoadDisk(dnd_container, 0); };
         popup_associated_function_ = std::bind(fn, &emulation_, dnd_container);
         AskForSaving(0);
         /*if (!AskForSaving(0))
         {
            emulation_.LoadDisk(dnd_container, 0);
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


         emulation_.LoadTape(list[0]);
         //UpdateStatusBar();
         break;
      }
      case 5:
         emulation_.LoadSnr(paths[0]);
         break;
      case 6:
         emulation_.LoadBin(paths[0]);
         break;
      case 8:
         emulation_.LoadCpr(paths[0]);
         break;
      }
   }
   
}

void SugarboxApp::KeyboardHandler(int key, int scancode, int action, int mods)
{
   // Handle shortcuts

   // Send scancode to emulation
   if (action == GLFW_PRESS)
   {
      emulation_.GetKeyboardHandler()->SendScanCode(key, true);
   }

   if (action == GLFW_RELEASE)
   {
      emulation_.GetKeyboardHandler()->SendScanCode(key, false);
   }
}

bool SugarboxApp::AskForSaving(int drive)
{
   if (emulation_.GetEngine()->IsDiskModified(drive))
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
   glfwSetWindowShouldClose(window_, true);
}

bool SugarboxApp::PauseEnabled()
{
   return emulation_.EmulationRun() == false;
}

void SugarboxApp::Pause()
{
   emulation_.Pause( );
}

void SugarboxApp::SetSpeed(int speedlimit)
{
   emulation_.GetEngine()->SetSpeed(speedlimit);
}

void SugarboxApp::HardReset()
{
   emulation_.HardReset();
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
   return emulation_.IsDiskPresent(drive);
}

void SugarboxApp::SaveAs(int drive)
{
   // Todo : Generic types, multilanguage, etc.
   ImGuiFileDialog::Instance()->OpenDialog("SaveAs", "Save as...", write_disk_extension_, ".");
   file_dialog_type_ = FD_SAVE_AS;
}

void SugarboxApp::Eject(int drive)
{
   // save ?
   popup_associated_function_ = std::bind (&EmulatorEngine::Eject, emulation_.GetEngine(), drive);
   AskForSaving(drive);
}

void SugarboxApp::Flip(int drive)
{
   emulation_.GetEngine()->FlipDisk(drive);
}

void SugarboxApp::InsertSelectFile(int drive)
{
   ImGuiFileDialog::Instance()->OpenDialog("SaveAs", "Insert disk...", load_disk_extension_, ".");
   file_dialog_type_ = FD_INSERT;
}

void SugarboxApp::InsertSelectTape()
{
   ImGuiFileDialog::Instance()->OpenDialog("SaveAs", "Insert tape...", load_tape_extension_, ".");
   file_dialog_type_ = FD_INSERT_TAPE;
}

void SugarboxApp::Insert(int drive)
{
   popup_associated_function_ = std::bind(&SugarboxApp::InsertSelectFile, this, drive);
   AskForSaving(drive);
}

void SugarboxApp::InsertBlankDisk(int drive, IDisk::DiskType type)
{
   emulation_.InsertBlankDisk(drive, type);
}

void SugarboxApp::InsertBlank(int drive, IDisk::DiskType type)
{
   popup_associated_function_ = std::bind(&SugarboxApp::InsertBlankDisk, this, drive, type);
   AskForSaving(drive);

}

void SugarboxApp::TapeRecord()
{
   emulation_.TapeRecord();
}

void SugarboxApp::TapePlay()
{
   emulation_.TapePlay();
}

void SugarboxApp::TapeFastForward()
{
   emulation_.TapeFastForward();
}

void SugarboxApp::TapeRewind()
{
   emulation_.TapeRewind();
}

void SugarboxApp::TapePause()
{
   emulation_.TapePause();
}

void SugarboxApp::TapeStop()
{
   emulation_.TapeStop();
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

   ImGuiFileDialog::Instance()->OpenDialog("SaveAs", "Save Tape as...", format_ext, ".");
   file_dialog_type_ = FD_SAVE_TAPE_AS;
   format_ = format;
}

void SugarboxApp::SnaLoad()
{
   ImGuiFileDialog::Instance()->OpenDialog("SaveAs", "Load snapshot", ".sna\0", ".");
   file_dialog_type_ = FD_INSERT_SNA;
}
