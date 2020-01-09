
#include "Display.h"

#include <SFML/Window.hpp>

#define REAL_DISP_X  1024 //832 //1024 // 768
#define REAL_DISP_Y  624 //-16 //624 //576

#define DISP_WIDTH    1024
#define DISP_HEIGHT   624

#define DISP_WINDOW_X   800
#define DISP_WINDOW_Y   600


CDisplay::CDisplay () : window_(nullptr), renderTexture_(nullptr), framebuffer_(nullptr), framebufferArray_(nullptr)
{
}

CDisplay::~CDisplay()
{
   delete renderTexture_;
   delete framebuffer_;
   delete window_;
   delete[]framebufferArray_;

}

unsigned int CDisplay::ConvertRGB(unsigned int rgb)
{
   return  (0xFF000000 | ((rgb & 0xFF)<<16) | ((rgb & 0xFF00) ) | ((rgb & 0xFF0000)>>16));
}

int CDisplay::GetWidth ()
{
   return m_Width;
}

int CDisplay::GetHeight ()
{
   return m_Height; //REAL_DISP_Y;
}

int* CDisplay::GetVideoBuffer (int y )
{
   return &framebufferArray_[ REAL_DISP_X * y*2];
}

void CDisplay::Reset () 
{
   memset( framebufferArray_ , 0, REAL_DISP_X * REAL_DISP_Y*4);
}

void CDisplay::Show ( bool bShow )
{
   m_bShow = bShow;
   if (window_)
      window_->setVisible(bShow);
}

void CDisplay::Init (sf::RenderWindow* window)
{
   window_ = window;
   framebuffer_ = new sf::Texture();
   renderTexture_ = new sf::RenderTexture();
   framebufferArray_ = new int[REAL_DISP_X * REAL_DISP_Y ];
   if (!framebuffer_->create(REAL_DISP_X, REAL_DISP_Y))
   {
      // error...
   }

   Reset();

}

void CDisplay::HSync ()
{
}

void CDisplay::StartSync()
{

}

void CDisplay::VSync (bool bDbg)
{
   framebuffer_->update((const sf::Uint8*)framebufferArray_);

   sf::Sprite sprite;
   sprite.setTexture(*framebuffer_);
   sprite.setTextureRect(sf::IntRect(143, 47, 680, 500));

   window_->draw(sprite);
   window_->display();

   sf::Event event;
   while (window_->pollEvent(event))
   {
   }
}

void CDisplay::WaitVbl ()
{
}

