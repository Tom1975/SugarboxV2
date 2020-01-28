#ifdef WIN32
   #pragma comment(linker, " /ENTRY:mainCRTStartup")
#endif

#include "Emulation.h"
#include "Display.h"
#include "SFML/Graphics.hpp"

int main()
{

   Emulation emulation;
   CDisplay display;

   // Create the main window
   sf::RenderWindow window(sf::VideoMode(640, 480), "Sugarbox v2");

   display.Init(&window);
   emulation.Init(&display);

   while (window.isOpen())
   {
      // Process events
      sf::Event event;
      while (window.pollEvent(event))
      {
         // Close window: exit
         if (event.type == sf::Event::Closed)
            window.close();
      }
      // Clear screen
      //window.clear();
      //window.display();

      display.Display();
   }

   emulation.Stop();

   return EXIT_SUCCESS;
}

