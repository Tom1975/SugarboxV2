#include "SugarboxApp.h"
#include "imgui.h"
#include "imgui-SFML.h"

SugarboxApp::SugarboxApp()
{

}

SugarboxApp::~SugarboxApp()
{

}

int SugarboxApp::RunApp()
{
   // Create the main window
   sf::RenderWindow window(sf::VideoMode(640, 480), "Sugarbox v2");

   display.Init(&window);
   emulation.Init(&display);
   IKeyboard* keyboard_handler = emulation.GetKeyboardHandler();

   ImGui::SFML::Init(window);

   sf::Clock deltaClock;

   while (window.isOpen())
   {
      // Process events
      sf::Event event;
      while (window.pollEvent(event))
      {
         ImGui::SFML::ProcessEvent(event);

         // Close window: exit
         if (event.type == sf::Event::Closed)
            window.close();

         //Respond to key pressed events
         if (event.type == sf::Event::EventType::KeyPressed) 
         {
            keyboard_handler->SendScanCode(event.key.code, true);
         }
         if (event.type == sf::Event::EventType::KeyReleased)
         {
            keyboard_handler->SendScanCode(event.key.code, false);
         }
      }
      ImGui::SFML::Update(window, deltaClock.restart());
      ImGui::Begin("Sugarbox", nullptr, ImGuiWindowFlags_NoDecoration| ImGuiWindowFlags_NoMove);
      display.Display(); 
      ImGui::Image(display.GetTexture());
      
      ImGui::End();

      
      ImGui::SFML::Render(window);
      window.display();
   }

   emulation.Stop();

   window.close();
   ImGui::SFML::Shutdown();

   return 0;
}