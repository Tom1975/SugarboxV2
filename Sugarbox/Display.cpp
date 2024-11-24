
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QMouseEvent>
#include <QImageWriter>
#include <QPainter>

#include <thread>

#include "stdafx.h"

#include "Display.h"


#define REAL_DISP_X  1024 //832 //1024 // 768
#define REAL_DISP_Y  624 //-16 //624 //576

//#define ORIGIN_X     143.f
#define ORIGIN_X     193.f
#define ORIGIN_Y     47.f

#define DISP_WIDTH    1024
#define DISP_HEIGHT   624

#define DISP_WINDOW_X   768
#define DISP_WINDOW_Y   544



CDisplay::CDisplay(QWidget *parent) : current_texture_(0), current_index_of_index_to_display_(0), number_of_frame_to_display_(0), sync_on_frame_(false),
   frame_emitted_ (0)
{
   QSurfaceFormat format;
   format.setSwapBehavior(QSurfaceFormat::DoubleBuffer); // Double buffering
   format.setSwapInterval(1); // Activer VSync
   //format.setVersion(3, 3); // Utiliser une version moderne si nécessaire
   QSurfaceFormat::setDefaultFormat(format);
   setFormat(format);

   setFocusPolicy(Qt::StrongFocus);
   memset(index_to_display_, 0, sizeof index_to_display_);
   setAutoFillBackground(false);

   buffer_list_ = new FrameItem[NB_FRAMES];
   buffer_list_[0].Init();
   buffer_list_[0].status_ = FrameItem::IN_USE;
   buffer_list_[0].sample_number_ = sample_number_++;
   index_current_buffer_ = 0;
   
   for (int i = 1; i < NB_FRAMES; i++)
   {
      buffer_list_[i].Init();
   }

}

CDisplay::~CDisplay()
{
   delete[]buffer_list_;
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
   //return &((framebufferArray_[current_texture_])[ REAL_DISP_X * y]);
   return &((buffer_list_[index_current_buffer_].framebufferArray_)[REAL_DISP_X * y]);
}

void CDisplay::Reset ()
{
   memset(buffer_list_[index_current_buffer_].framebufferArray_, 0, REAL_DISP_X * REAL_DISP_Y*4);
}

void CDisplay::Show ( bool bShow )
{
}

void CDisplay::Init()
{
      //framebufferArray_[i] = new int[1024 * 1024];
   // grabKeyboard();
}

void CDisplay::initializeGL()
{
   initializeOpenGLFunctions();

   glDisable(GL_MULTISAMPLE);

   static const int coords[4][2] = {
      { +1, -1/*, -1*/ },
      { -1, -1/*, -1*/ },
      { -1, +1/*, -1*/ },
      { +1, +1/*, -1*/ },
   };

   textures[0] = new QOpenGLTexture(QOpenGLTexture::Target2D);
   textures[0]->setMinMagFilters(QOpenGLTexture::Nearest, QOpenGLTexture::Nearest);
   textures[0]->create();

   // given some `width`, `height` and `data_ptr`
   textures[0]->setSize(1024, 1024, 1);
   textures[0]->setFormat(QOpenGLTexture::RGBA8_UNorm);
   textures[0]->allocateStorage();

   QVector<GLfloat> vertData;

   float ratiox = (float)DISP_WINDOW_X / (float)REAL_DISP_X;
   float ratioy = (float)DISP_WINDOW_Y / (float)REAL_DISP_X;

   for (int j = 0; j < 4; ++j) {
      // vertex position
      vertData.append(coords[j][0]);
      vertData.append(coords[j][1]);
      // texture coordinate
      vertData.append(j == 0 || j == 3);
      vertData.append(j == 0 || j == 1);
      // Origin
      vertData.append(ORIGIN_X / 1024 * ratiox);
      vertData.append(ORIGIN_Y / 1024 * ratioy);
      // Ratio
      vertData.append(ratiox);
      vertData.append(ratioy);
   }

   vbo.create();
   vbo.bind();
   vbo.allocate(vertData.constData(), vertData.count() * sizeof(GLfloat));

#define PROGRAM_VERTEX_ATTRIBUTE 0
#define PROGRAM_TEXCOORD_ATTRIBUTE 1

   QString vertexShader =
      "attribute vec4 aPosition;\n"
      "attribute vec2 aTexCoord;\n"
      "attribute vec2 origin;\n"
      "attribute vec2 ratio;\n"
      "varying vec2 vTexCoord;\n"
      "varying vec2 vorigin;\n"
      "varying vec2 vratio;\n"

      "void main()\n"
      "{\n"
      "   gl_Position = aPosition;\n"
      "   gl_Position.y = gl_Position.y;\n"
      "   vTexCoord.x = aTexCoord.x;\n"
      "   vTexCoord.y = aTexCoord.y/2.0;\n"
      "   vorigin = origin;\n"
      "   vratio = ratio;\n"
      "}";

   QString fragmentShader =
      "uniform sampler2D texture;\n"
      "varying vec2 vTexCoord;\n"
      "varying vec2 vorigin;\n"
      "varying vec2 vratio;\n"
      "void main()\n"
      "{\n"
      "   vec2 coord = vec2( vTexCoord.x *vratio.x + vorigin.x, vTexCoord.y*vratio.y + vorigin.y);\n"
      "   gl_FragColor = texture2D(texture, coord);\n"
      "}";

   QOpenGLShader *vshader = new QOpenGLShader(QOpenGLShader::Vertex);
   vshader->compileSourceCode(vertexShader);

   QOpenGLShader *fshader = new QOpenGLShader(QOpenGLShader::Fragment);
   fshader->compileSourceCode(fragmentShader);

   program = new QOpenGLShaderProgram;
   program->addShader(vshader);
   program->addShader(fshader);
   program->bindAttributeLocation("aPosition", 0);
   program->bindAttributeLocation("aTexCoord", 1);
   program->bindAttributeLocation("origin", 2);
   program->bindAttributeLocation("ratio", 3);

   program->link();

   program->bind();
   program->setUniformValue("texture", 0);
}

void CDisplay::HSync ()
{
}

void CDisplay::StartSync()
{

}

void CDisplay::Screenshot()
{
   // Take a screenshot : Last displayed

   QImage img((unsigned char*)buffer_list_[0].framebufferArray_, 1024, 1024, QImage::Format_ARGB32);
   QImage scr(DISP_WINDOW_X, DISP_WINDOW_Y, QImage::Format_ARGB32);

   QPainter p;
   for (int line = ORIGIN_Y; line < DISP_WINDOW_Y / 2; line++)
   {
      p.begin(&scr);
      p.drawImage(QRectF(0, line * 2, DISP_WINDOW_X, 2),
         img, QRectF(ORIGIN_X, line, DISP_WINDOW_X, 1),
         Qt::AutoColor);
      p.end();
   }

   scr.save("SCREENSHOT.JPG");
}

void CDisplay::Screenshot(const char* scr_path)
{
   // Take a screenshot : Last displayed

   QImage img((unsigned char*)buffer_list_[0].framebufferArray_, 1024, 1024, QImage::Format_ARGB32);
   QImage scr(DISP_WINDOW_X, DISP_WINDOW_Y, QImage::Format_ARGB32);

   QPainter p;
   for (int line = ORIGIN_Y; line < DISP_WINDOW_Y/2; line++)
   {
      p.begin(&scr);
      p.drawImage(QRectF(0, line*2, DISP_WINDOW_X, 2),
         img, QRectF(ORIGIN_X, line, DISP_WINDOW_X, 1),
         Qt::AutoColor);
      p.end();
   }

   scr.save(scr_path);


}

void CDisplay::SyncOnFrame(bool set)
{
   sync_on_frame_ = set;
   // Restart sync : We try to avoid deadlocks
}

bool CDisplay::IsWaitHandled()
{
   // Wait, depending on the selected value
   // VBL : wait for next VBL
   glFinish();

   return true;
};

void CDisplay::VSync (bool bDbg)
{
   int free_buffer = 0;
   int next_to_play = -1;
   int count_deadlock = 0;

   do
   {
      sync_mutex_.lock();
      for (int i = 0; i < NB_FRAMES && next_to_play == -1; i++)
      {
         if (buffer_list_[i].status_ == FrameItem::FREE)
         {
            next_to_play = i;
            //break;
         }
      }
      sync_mutex_.unlock();
      if (sync_on_frame_ && next_to_play == -1 )
      {
         count_deadlock++;
         std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
   } while (sync_on_frame_ && next_to_play == -1 && count_deadlock < 100);
   
   if (next_to_play != -1)
   {
      // Buffer is full ? Prepare next, and mark this one to be played
      buffer_list_[index_current_buffer_].status_ = FrameItem::TO_PLAY;
      sync_mutex_.lock();
      frame_emitted_++;
      sync_mutex_.unlock();
      emit FrameIsReady();
      index_current_buffer_ = next_to_play;
   }
   else
   {
      // What ?!!!
      int dbg = 1;
   }
   buffer_list_[index_current_buffer_].status_ = FrameItem::IN_USE;
   buffer_list_[index_current_buffer_].sample_number_ = sample_number_++;
}

void CDisplay::Display()
{
   update();
}

void CDisplay::WaitVbl ()
{
}

void CDisplay::paintGL()
{
   // Signal received
   sync_mutex_.lock();
   frame_emitted_--;
   sync_mutex_.unlock();

   int index_to_convert = -1;
   int sample_number = -1;
   sync_mutex_.lock();
   for (int i = 0; i < NB_FRAMES; i++)
   {
      if (buffer_list_[i].status_ == FrameItem::TO_PLAY
         && (sample_number == -1 || buffer_list_[i].sample_number_ <= sample_number))
      {
         sample_number = buffer_list_[i].sample_number_;
         index_to_convert = i;
      }
   }

   if (index_to_convert != -1)
   {
      buffer_list_[index_to_convert].status_ = FrameItem::LOCKED;
      sync_mutex_.unlock();
   //if (number_of_frame_to_display_ > 0)
   //{
      //textures[0]->setData(QOpenGLTexture::BGRA, QOpenGLTexture::UInt8, (unsigned char*)framebufferArray_[0]);
      textures[0]->setData(QOpenGLTexture::BGRA, QOpenGLTexture::UInt8, (unsigned char*)buffer_list_[index_to_convert].framebufferArray_);
      

      glDisable(GL_MULTISAMPLE);
      glClearColor(clearColor.redF(), clearColor.greenF(), clearColor.blueF(), clearColor.alphaF());
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      //program->setUniformValue("matrix", m);
      program->enableAttributeArray(PROGRAM_VERTEX_ATTRIBUTE);
      program->enableAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE);
      program->enableAttributeArray(2);
      program->enableAttributeArray(3);
      program->setAttributeBuffer(PROGRAM_VERTEX_ATTRIBUTE, GL_FLOAT, 0, 2, 8 * sizeof(GLfloat));
      program->setAttributeBuffer(PROGRAM_TEXCOORD_ATTRIBUTE, GL_FLOAT, 2 * sizeof(GLfloat), 2, 8 * sizeof(GLfloat));
      // origin
      program->setAttributeBuffer(2, GL_FLOAT, 4 * sizeof(GLfloat), 2, 8 * sizeof(GLfloat));
      // ratio
      program->setAttributeBuffer(3, GL_FLOAT, 6 * sizeof(GLfloat), 2, 8 * sizeof(GLfloat));

      textures[0]->bind();
      glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

      // Put it back to 'free_buffers_'
      buffer_list_[index_to_convert].status_ = FrameItem::FREE;

      // Wait vsync if vsync needed
      if (sync_on_frame_)
      {

         const QSurfaceFormat fmt = format();
         int swp = fmt.swapInterval();
         glFinish();

      }
      
   }
   else
   {
      // Redraw old frame ? (or do nothing !)
      sync_mutex_.unlock();
   }
}
