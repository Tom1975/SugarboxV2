
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QMouseEvent>

#include "Display.h"


#define REAL_DISP_X  1024 //832 //1024 // 768
#define REAL_DISP_Y  624 //-16 //624 //576

#define ORIGIN_X     143.f
#define ORIGIN_Y     47.f

#define DISP_WIDTH    1024
#define DISP_HEIGHT   624

#define DISP_WINDOW_X   768
#define DISP_WINDOW_Y   544


CDisplay::CDisplay(QWidget *parent) : current_texture_(0), current_index_of_index_to_display_(0), number_of_frame_to_display_(0)
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



}

void CDisplay::initializeGL()
{
   char log[256];
   for (int i = 0; i < NB_FRAMES; i++)
   {
      framebufferArray_[i] = new int[1024 * 1024];
   }

   initializeOpenGLFunctions();

   static const int coords[4][3] = {
          { +1, -1, -1 }, { -1, -1, -1 }, { -1, +1, -1 }, { +1, +1, -1 },
   };

   //QImage img((unsigned char*)framebufferArray_[0], 1024, 1024, 1024*4, QImage::Format_ARGB32);
   textures[0] = new QOpenGLTexture(QOpenGLTexture::Target2D);
   textures[0]->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
   textures[0]->create();

   // given some `width`, `height` and `data_ptr`
   textures[0]->setSize(1024, 1024, 1);
   textures[0]->setFormat(QOpenGLTexture::RGBA8_UNorm);
   textures[0]->allocateStorage();

   QVector<GLfloat> vertData;
   for (int j = 0; j < 4; ++j) {
      // vertex position
      vertData.append(0.2 * coords[j][0]);
      vertData.append(0.2 * coords[j][1]);
      vertData.append(0.2 * coords[j][2]);
      // texture coordinate
      vertData.append(j == 0 || j == 3);
      vertData.append(j == 0 || j == 1);
   }

   vbo.create();
   vbo.bind();
   vbo.allocate(vertData.constData(), vertData.count() * sizeof(GLfloat));

   glEnable(GL_DEPTH_TEST);
   glEnable(GL_CULL_FACE);

#define PROGRAM_VERTEX_ATTRIBUTE 0
#define PROGRAM_TEXCOORD_ATTRIBUTE 1

   QOpenGLShader *vshader = new QOpenGLShader(QOpenGLShader::Vertex, this);
   const char *vsrc =
      "attribute highp vec4 vertex;\n"
      "attribute mediump vec4 texCoord;\n"
      "varying mediump vec4 texc;\n"
      "uniform mediump mat4 matrix;\n"
      "void main(void)\n"
      "{\n"
      "    gl_Position = matrix * vertex;\n"
      "    texc = texCoord;\n"
      "}\n";
   vshader->compileSourceCode(vsrc);

   QOpenGLShader *fshader = new QOpenGLShader(QOpenGLShader::Fragment, this);
   const char *fsrc =
      "uniform sampler2D texture;\n"
      "varying mediump vec4 texc;\n"
      "void main(void)\n"
      "{\n"
      "    gl_FragColor = texture2D(texture, texc.st);\n"
      "}\n";
   fshader->compileSourceCode(fsrc);

   program = new QOpenGLShaderProgram;
   program->addShader(vshader);
   program->addShader(fshader);
   program->bindAttributeLocation("vertex", PROGRAM_VERTEX_ATTRIBUTE);
   program->bindAttributeLocation("texCoord", PROGRAM_TEXCOORD_ATTRIBUTE);
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

void CDisplay::VSync (bool bDbg)
{
   // Add a frame to display, if display buffer is not full !
   //if (number_of_frame_to_display_ < NB_FRAMES)
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
      //QImage img((unsigned char*)framebufferArray_[0], 1024, 1024, 1024 * 4, QImage::Format_ARGB32);
      //textures[0] = new QOpenGLTexture(img);
      textures[0]->setData(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, (unsigned char*)framebufferArray_[0]);
   }

}

void CDisplay::WaitVbl ()
{
}

void CDisplay::paintGL()
{
   Display();

   glClearColor(clearColor.redF(), clearColor.greenF(), clearColor.blueF(), clearColor.alphaF());
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   QMatrix4x4 m;
   //m.ortho(-0.5f, +0.5f, +0.5f, -0.5f, 4.0f, 15.0f);
   m.ortho(-0.0f, +0.5f, +0.5f, -0.5f, 4.0f, 15.0f);

   m.translate(0.0f, 0.0f, -8.0f);
   /*m.rotate(xRot / 16.0f, 1.0f, 0.0f, 0.0f);
   m.rotate(yRot / 16.0f, 0.0f, 1.0f, 0.0f);
   m.rotate(zRot / 16.0f, 0.0f, 0.0f, 1.0f);*/

   program->setUniformValue("matrix", m);
   program->enableAttributeArray(PROGRAM_VERTEX_ATTRIBUTE);
   program->enableAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE);
   program->setAttributeBuffer(PROGRAM_VERTEX_ATTRIBUTE, GL_FLOAT, 0, 3, 5 * sizeof(GLfloat));
   program->setAttributeBuffer(PROGRAM_TEXCOORD_ATTRIBUTE, GL_FLOAT, 3 * sizeof(GLfloat), 2, 5 * sizeof(GLfloat));

   textures[0]->bind();
   glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}