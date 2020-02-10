#include "SugarboxApp.h"


#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <GL/gl3w.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>


SugarboxApp::SugarboxApp() : counter_(0), str_speed_("0%"), keyboard_handler_(nullptr)
{
   
}

SugarboxApp::~SugarboxApp()
{
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
   int i;
   for (i = 0; i < count; i++)
   {

   }
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
   if (key == GLFW_KEY_E && action == GLFW_PRESS)
   {


   }
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

int SugarboxApp::RunApp()
{
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
   glfwMakeContextCurrent(window_);
   glfwSetKeyCallback(window_, KeyCallback);
   glfwSetDropCallback(window_, DropCallback);
   glfwSetJoystickCallback(joystick_callback);

   bool err = gl3wInit() != 0;

   // Init sound
   // TODO
   
   // Init Gui
   ImGui::CreateContext();
   ImGui_ImplGlfw_InitForOpenGL(window_, true);
   const char* glsl_version = "#version 130";
   ImGui_ImplOpenGL3_Init(glsl_version);
   
   // Init display
   glViewport(0, status_height, main_display_width, main_display_height);

   display_.Init();
   emulation_.Init(&display_, this);
   keyboard_handler_ = emulation_.GetKeyboardHandler();

   // Run main loop
   RunMainLoop();

   glfwDestroyWindow(window_);
   glfwTerminate();

   return 0;
}



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
   
   /*
   ImGui::SetNextWindowPos(ImVec2(0, toolbar_height), ImGuiCond_Always);
   ImGui::SetNextWindowSize(ImVec2(main_display_width, main_display_height), ImGuiCond_Always);
   ImGui::Begin("Sugarbox", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_MenuBar);
   display_.Display();
   ImGui::Image((void*)(intptr_t)display_.GetTexture(), ImVec2(1024, 1024) );
   ImGui::End();
   */
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
         if (ImGui::MenuItem("Open..", "Ctrl+O")) { /* Do stuff */ }
         if (ImGui::MenuItem("Save", "Ctrl+S")) { /* Do stuff */ }
         if (ImGui::MenuItem("Close", "Ctrl+W")) {}
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
