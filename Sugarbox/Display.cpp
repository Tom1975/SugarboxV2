
#include "Display.h"

#include <SFML/Window.hpp>

#define REAL_DISP_X  1024 //832 //1024 // 768
#define REAL_DISP_Y  624 //-16 //624 //576

#define DISP_WIDTH    1024
#define DISP_HEIGHT   624

#define DISP_WINDOW_X   800
#define DISP_WINDOW_Y   600

const std::string std_shader = "uniform sampler2D texture;"\
"void main()"\
"{"\
"   vec2 coord = vec2((gl_TexCoord[0].x * 768.0 + 143.0) / 1024.0, (gl_TexCoord[0].y * 500.0 + 23.0) / 1024.0);"\
"   vec4 pixel = texture2D(texture, coord);"\
"   gl_FragColor = gl_Color * pixel;"\
"}";

CDisplay::CDisplay () : window_(nullptr), framebuffer_(nullptr), current_texture_(0), current_index_of_index_to_display_(0), number_of_frame_to_display_(0)
{
   memset(index_to_display_, 0, sizeof index_to_display_);
}

CDisplay::~CDisplay()
{
   delete framebuffer_;
   for (int i = 0; i < NB_FRAMES; i++)
   {
      delete[]framebufferArray_[i];
   }

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
   return &((framebufferArray_[current_texture_])[ REAL_DISP_X * y]);
}

void CDisplay::Reset () 
{
   memset( framebufferArray_[current_texture_], 0, REAL_DISP_X * REAL_DISP_Y*4);
}

void CDisplay::Show ( bool bShow )
{
   if (window_)
      window_->setVisible(bShow);
}

void CDisplay::Init (sf::RenderWindow* window)
{
   window_ = window;

   framebuffer_ = new sf::Texture();
   if (!framebuffer_->create(REAL_DISP_X, REAL_DISP_Y))
   {
      // error...
   }

   for (int i = 0; i < NB_FRAMES; i++)
   {
      framebufferArray_[i] = new int[REAL_DISP_X * REAL_DISP_Y];
   }

   // Create shaders
   if (standard_display_shader_.loadFromMemory(std_shader, sf::Shader::Fragment))
   {
      if (standard_display_shader_.isAvailable())
      {
         standard_display_shader_.setUniform("texture", sf::Shader::CurrentTexture);
      }
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
   // Add a frame to display, if display buffer is not full !
   if (number_of_frame_to_display_ < NB_FRAMES)
   {
      index_to_display_[(current_index_of_index_to_display_ + number_of_frame_to_display_) % NB_FRAMES] = current_texture_;
      current_texture_ = (current_texture_ + 1) % NB_FRAMES;
      number_of_frame_to_display_++;
   }
}

void CDisplay::Display()
{
   if (number_of_frame_to_display_ > 0)
   {
      // Get next available texture
      framebuffer_->update((const sf::Uint8*)framebufferArray_[index_to_display_[current_index_of_index_to_display_]]);

      sf::Sprite sprite;
      sprite.setTexture(*framebuffer_);
      //sprite.setTextureRect(sf::IntRect(143, 23, 680, 250));
      //sprite.setTextureRect(sf::IntRect(143, 47, 680, 500));
      sprite.setTextureRect(sf::IntRect(0, 0, 1024, 1024));

      window_->draw(sprite, &standard_display_shader_);
      window_->display();

      current_index_of_index_to_display_ = (current_index_of_index_to_display_ + 1) % NB_FRAMES;
      number_of_frame_to_display_--;
   }

}

void CDisplay::WaitVbl ()
{
}
