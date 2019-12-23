#include "imgui.h"
#include "imgui-SFML.h"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Graphics/CircleShape.hpp>

int main()
{
   sf::RenderWindow window(sf::VideoMode(640, 480), "Debugger");
   sf::RenderWindow sbx(sf::VideoMode(640, 480), "Sugarbox");

//   sf::RenderWindow dbg_wnd(sf::VideoMode(640, 480), "Debugger");
   //window.setFramerateLimit(50);
   ImGui::SFML::Init(window);
//   ImGui::SFML::Init(dbg_wnd);

   sf::Clock deltaClock;
   while (sbx.isOpen()) {
      sf::Event event;
      while (sbx.pollEvent(event)) {
         ImGui::SFML::ProcessEvent(event);

         if (event.type == sf::Event::Closed) {
            sbx.close();
         }
      }

      ImGui::SFML::Update(window, deltaClock.restart());

      bool my_tool_active = true;
      ImGui::Begin("My First Tool", &my_tool_active, ImGuiWindowFlags_MenuBar);
      if (ImGui::BeginMenuBar())
      {
         if (ImGui::BeginMenu("File"))
         {
            if (ImGui::MenuItem("Open..", "Ctrl+O")) { /* Do stuff */ }
            if (ImGui::MenuItem("Save", "Ctrl+S")) { /* Do stuff */ }
            if (ImGui::MenuItem("Close", "Ctrl+W")) { my_tool_active = false; }
            ImGui::EndMenu();
         }
         ImGui::EndMenuBar();
      }
      ImGui::End();


      ImGui::Begin("Hello, world!");
      ImGui::Button("Look at this pretty button");
      ImGui::End();

      window.clear();
      ImGui::SFML::Render(window);
      window.display();
   }

   ImGui::SFML::Shutdown();
}