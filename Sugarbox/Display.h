#pragma once

// C RunTime Header Files
#include <stdlib.h>
#include <string>
#include <malloc.h>
#include <memory.h>
#include "Screen.h"

// SFML
#include <SFML/Graphics.hpp>

#define NB_FRAMES 3
// Display
class CDisplay : public IDisplay
{
public :
   CDisplay ();
   virtual ~CDisplay ();

   void Init(sf::RenderWindow* window);
   void Show(bool bShow);
   void Display();

   virtual unsigned int ConvertRGB(unsigned int rgb);
   virtual void SetScanlines ( int scan ) {};
   virtual bool AFrameIsReady  () {return true;};
   virtual void Config () {};
   virtual const char* GetInformations () { return "TCL GDI";};
   virtual int GetWidth () ;
   virtual int GetHeight () ;
   virtual void HSync () ;
   virtual void VSync (bool bDbg) ;
   virtual void StartSync();
   virtual void WaitVbl () ;
   virtual int* GetVideoBuffer (int y) ;
   virtual void Reset () ;
   virtual void Screenshot (){};
   virtual void ScreenshotEveryFrame(int bSetOn) {};
   virtual bool IsEveryFrameScreened() {
      return false;
   }
   virtual bool IsDisplayed() { return true;/* m_bShow;*/ };
   virtual void FullScreenToggle (){};
   virtual void ForceFullScreen (bool bSetFullScreen ){}
   virtual void WindowChanged (int xIn, int yIn, int wndWidth, int wndHeight){};
   virtual bool SetSyncWithVbl ( int speed ){return false; };
   virtual bool IsWaitHandled() { return false; };
   virtual bool GetBlackScreenInterval () { return false ;};
   virtual void SetBlackScreenInterval (bool bBS) { };

   virtual void SetSize (SizeEnum size){};
   virtual SizeEnum  GetSize () { return S_STANDARD; };


   virtual void ResetLoadingMedia() {};
   virtual void SetLoadingMedia() {};

   virtual void ResetDragnDropDisplay () {};
   virtual void SetDragnDropDisplay  ( int type ){};
   virtual void SetCurrentPart (int x, int y ){};
   virtual int GetDnDPart () { return 0;};

protected:

   // Displayed window : 
   int m_X, m_Y;
   int m_Width;
   int m_Height;

   // Window
   sf::RenderWindow* window_;

   // Textures
   sf::Texture* framebuffer_;
   int* framebufferArray_[NB_FRAMES];

   // Textures to display indexes
   int index_to_display_[NB_FRAMES];
   int current_index_of_index_to_display_;
   int number_of_frame_to_display_;

   // Texture Indexes
   int current_texture_;

   // Shader
   sf::Shader standard_display_shader_;
};
