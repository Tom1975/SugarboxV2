#include "SugarboxApp.h"
#include "imgui.h"

SugarboxApp::SugarboxApp() : counter_(0), str_speed_("0%"), keyboard_handler_(nullptr)
{
   
}

SugarboxApp::~SugarboxApp()
{
}


//////////////////////////////////////////////
/// ISoundFactory interface
ISound* SugarboxApp::GetSound(const char* name)
{
   return &sound_mixer_;
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
   // Create the main window
   window_width_ = main_display_width + peripherals_width;
   window_height_ = main_display_height + toolbar_height + status_height;


   // Init display
   display_.Init();
   emulation_.Init(&display_, this);
   keyboard_handler_ = emulation_.GetKeyboardHandler();

   // Init sound

   
   // Init Gui


   // Run main loop
   RunMainLoop();

   return 0;
}

void SugarboxApp::RunMainLoop()
{
/*   // Run debugger thread
   while (window_->isOpen())
   {
      // Process events
      sf::Event event;
      while (window_->pollEvent(event))
      {
         ImGui::SFML::ProcessEvent(event);

         // Close window: exit
         if (event.type == sf::Event::Closed)
            window_->close();

         //Respond to key pressed events
         if (event.type == sf::Event::EventType::KeyPressed) 
         {
            keyboard_handler_->SendScanCode(event.key.code, true);
         }
         if (event.type == sf::Event::EventType::KeyReleased)
         {
            keyboard_handler_->SendScanCode(event.key.code, false);
         }
      }
      ImGui::SFML::Update(*window_, deltaClock.restart());

      // Toolbar window
      DrawMenu();

      // Main window
      DrawMainWindow();

      // Peripherals window
      DrawPeripherals();

      // Status window
      DrawStatusBar();

      ImGui::SFML::Render(*window_);
      window_->display();
   }
   */
   emulation_.Stop();
   /*window_->close();
   ImGui::SFML::Shutdown();*/
}

void SugarboxApp::DrawMainWindow()
{
   ImGui::SetNextWindowPos(ImVec2(0, toolbar_height), ImGuiCond_Always);
   ImGui::SetNextWindowSize(ImVec2(main_display_width, main_display_height), ImGuiCond_Always);
   ImGui::Begin("Sugarbox", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_MenuBar);
   display_.Display();
   //ImGui::Image(display_.GetTexture());
   ImGui::End();

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
