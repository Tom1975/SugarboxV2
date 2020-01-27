#ifdef WIN32
   #pragma comment(linker, " /ENTRY:mainCRTStartup")
#endif

#include "Emulation.h"
#include "Display.h"
#include "SFML/Graphics.hpp"


void RunLoop(Emulation* emulator)
{
   emulator->EmulationLoop();
}

int main()
{

   Emulation emulation;
   CDisplay display;

   // Create the main window
   sf::RenderWindow window(sf::VideoMode(800, 600), "Sugarbox v2");

   display.Init(&window);
   emulation.Init(&display);

   std::thread* worker_thread = new std::thread(RunLoop, &emulation);



   // Start the game loop
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
   return EXIT_SUCCESS;
}

