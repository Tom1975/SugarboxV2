#include "SugarboxApp.h"

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

   while (window.isOpen())
   {
      // Process events
      sf::Event event;
      while (window.pollEvent(event))
      {
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

      display.Display();
   }

   emulation.Stop();

   return 0;
}