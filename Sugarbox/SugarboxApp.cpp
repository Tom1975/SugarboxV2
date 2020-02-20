#include "SugarboxApp.h"


#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <GL/gl3w.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

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

SugarboxApp::SugarboxApp() : counter_(0), str_speed_("0%"), keyboard_handler_(nullptr)
{
  
}

SugarboxApp::~SugarboxApp()
{
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
   language_.Init("Resources/labels.ini");
}

int SugarboxApp::RunApp()
{
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
   emulation_.Init(&display_, this);
   keyboard_handler_ = emulation_.GetKeyboardHandler();

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

void SugarboxApp::DrawMenu()
{
   ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
   ImGui::SetNextWindowSize(ImVec2(window_width_, toolbar_height), ImGuiCond_Always);

   ImGui::Begin("Toolbar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_MenuBar);
   if (ImGui::BeginMenuBar())
   {
      if (ImGui::BeginMenu("File"))
      {
         if (ImGui::MenuItem(language_.GetString("L_FILE_EXIT"), "Alt+F4")) { glfwSetWindowShouldClose(window_, true); }
         ImGui::EndMenu();
      }
      ImGui::EndMenuBar();
   }
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
      
      sprintf(str_speed_, "%i%%", emulation_.GetSpeed());
      counter_ = 0;
   }
   ImGui::Text(str_speed_);
   // Sound control

   ImGui::End();
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
            }
            if (ImGui::Button("No"))
            {
               emulation_.GetEngine()->SaveDisk(PopupArg);
               PopupType = POPUP_NONE;
            }
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
         test = true;
         AskForSaving(0);
         //m_SkipNextAutorun = false;
         //m_pMachine->LoadDisk (m_DragFiles[0], part);
         emulation_.LoadDisk(dnd_container, 0);

         break;
         // Tape - TODO
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
      emulation_.GetKeyboardHandler()->SendScanCode(scancode, true);
   }

   if (action == GLFW_RELEASE)
   {
      emulation_.GetKeyboardHandler()->SendScanCode(scancode, false);
   }
}

void SugarboxApp::AskForSaving(int drive)
{
   if (emulation_.GetEngine()->IsDiskModified(drive))
   {
      PopupType = POPUP_ASK_SAVE;
      PopupArg = drive;
   }
}