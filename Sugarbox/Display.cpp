
#include "Display.h"

#define REAL_DISP_X  1024 //832 //1024 // 768
#define REAL_DISP_Y  624 //-16 //624 //576

#define DISP_WIDTH    1024
#define DISP_HEIGHT   624

#define DISP_WINDOW_X   800
#define DISP_WINDOW_Y   600

const std::string std_shader = \
"uniform sampler2D texture;"\
"uniform vec2 origin;"\
"uniform vec2 size_of_display;"\
"uniform vec2 size_of_texture;"\
"void main()"\
"{"\
"   vec2 coord = vec2((gl_TexCoord[0].x * size_of_display.x + origin.x) / size_of_texture.x, ((1.0-gl_TexCoord[0].y) * size_of_display.y + origin.y) / size_of_texture.y);"\
"   vec4 pixel = texture2D(texture, coord);"\
"   gl_FragColor = gl_Color * pixel;"\
"}";
/*
static const vec2 vertices[4] =
{
    { 0.f, 0.f },
    { 1.f, 0.f },
    { 1.f, 1.f },
    { 0.f, 1.f }
};*/

CDisplay::CDisplay() : current_texture_(0), current_index_of_index_to_display_(0), number_of_frame_to_display_(0)
{
   memset(index_to_display_, 0, sizeof index_to_display_);
}

CDisplay::~CDisplay()
{
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
}

void CDisplay::Init ()
{
   for (int i = 0; i < NB_FRAMES; i++)
   {
      framebufferArray_[i] = new int[1024*1024];
   }

   glGenTextures(NB_FRAMES, texture_);
   glBindTexture(GL_TEXTURE_2D, texture_[0]);

   // Setup filtering parameters for display
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
   //glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, 1024, 1024, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, framebufferArray_[number_of_frame_to_display_]);
   
   for (int i = 0; i < 1024 * 1024; i++)
   {
      (framebufferArray_[0])[i] = (rand() & 0xFF) + ((rand() & 0xFF) << 8) + ((rand() & 0xFF) << 16) + ((rand() & 0xFF) << 24);
   }

   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1024, 1024, 0, GL_RGBA, GL_UNSIGNED_BYTE, framebufferArray_[0]);


   //Reset();
}

void CDisplay::HSync ()
{
}

void CDisplay::StartSync()
{

}

void CDisplay::VSync (bool bDbg)
{
   return;
   // Add a frame to display, if display buffer is not full !
   if (number_of_frame_to_display_ < NB_FRAMES)
   {
      index_to_display_[(current_index_of_index_to_display_ + number_of_frame_to_display_) % NB_FRAMES] = current_texture_;
      //current_texture_ = (current_texture_ + 1) % NB_FRAMES;
      number_of_frame_to_display_++;
   }
}

GLuint CDisplay::GetTexture()
{
   return texture_[0/* current_index_of_index_to_display_*/];
}

void CDisplay::Display()
{
   //if (number_of_frame_to_display_ > 0)
   {
     /* current_index_of_index_to_display_ = 1;

      glBindTexture(GL_TEXTURE_2D, texture_[current_index_of_index_to_display_]);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1024, 1024, 0, GL_BGR, GL_UNSIGNED_BYTE, framebufferArray_[current_index_of_index_to_display_]);

      current_index_of_index_to_display_ = (current_index_of_index_to_display_ + 1) % NB_FRAMES;
      number_of_frame_to_display_--;
      */
   }

}

void CDisplay::WaitVbl ()
{
}
