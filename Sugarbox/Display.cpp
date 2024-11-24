
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QMouseEvent>
#include <QImageWriter>
#include <QPainter>

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


CDisplay::CDisplay(QWidget *parent) : current_texture_(0), current_index_of_index_to_display_(0), number_of_frame_to_display_(0)
{
   setFocusPolicy(Qt::StrongFocus);
   memset(index_to_display_, 0, sizeof index_to_display_);
   setAutoFillBackground(false);
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
   // TODO TG : Beware of this, as it can prevent using keyboard in debug windows (for example)
   // grabKeyboard();
   for (int i = 0; i < NB_FRAMES; i++)
   {
      framebufferArray_[i] = new int[1024 * 1024];
   }
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

   QImage img((unsigned char*)framebufferArray_[0], 1024, 1024, QImage::Format_ARGB32);
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

   QImage img((unsigned char*)framebufferArray_[0], 1024, 1024, QImage::Format_ARGB32);
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

void CDisplay::VSync (bool bDbg)
{
   // Add a frame to display, if display buffer is not full !
   if (number_of_frame_to_display_ < NB_FRAMES)
   {
      //index_to_display_[(current_index_of_index_to_display_ + number_of_frame_to_display_) % NB_FRAMES] = current_texture_;
      index_to_display_[0] = current_texture_;
      //current_texture_ = (current_texture_ + 1) % NB_FRAMES;
      number_of_frame_to_display_=1;
      emit FrameIsReady();
   }
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

   if (number_of_frame_to_display_ > 0)
   {
      textures[0]->setData(QOpenGLTexture::BGRA, QOpenGLTexture::UInt8, (unsigned char*)framebufferArray_[0]);

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
   }
}
