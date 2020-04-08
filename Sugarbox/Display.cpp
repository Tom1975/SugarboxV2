
#include "Display.h"

#define REAL_DISP_X  1024 //832 //1024 // 768
#define REAL_DISP_Y  624 //-16 //624 //576

#define ORIGIN_X     143.f
#define ORIGIN_Y     47.f

#define DISP_WIDTH    1024
#define DISP_HEIGHT   624

#define DISP_WINDOW_X   768
#define DISP_WINDOW_Y   544


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
   return  (0xFF000000 | ((rgb & 0xFF) ) | ((rgb & 0xFF00) ) | ((rgb & 0xFF0000) ));
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

void CDisplay::Init()
{
   char log[256];
   for (int i = 0; i < NB_FRAMES; i++)
   {
      framebufferArray_[i] = new int[1024 * 1024];
   }


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
      //current_texture_ = (current_texture_ + 1) % NB_FRAMES;
      number_of_frame_to_display_++;
   }
}

void CDisplay::Display()
{
   if (number_of_frame_to_display_ > 0)
   {
   }

}

void CDisplay::WaitVbl ()
{
}
