
#include "Display.h"

#define REAL_DISP_X  1024 //832 //1024 // 768
#define REAL_DISP_Y  624 //-16 //624 //576

#define ORIGIN_X     143.f
#define ORIGIN_Y     47.f

#define DISP_WIDTH    1024
#define DISP_HEIGHT   624

#define DISP_WINDOW_X   768
#define DISP_WINDOW_Y   544

static const char* vertex_quad_shader_2 = \
"#version 150\n"
"in vec2 in_Vertex;"
"out vec2 texcoords;"
"void main() {"
"   gl_Position = vec4(in_Vertex, 0.0f, 1.0f);\n"
"   texcoords = in_Vertex + vec2(1.0, 1.0);\n"
"   texcoords.y = 0.5 - texcoords.y/4.0;\n"
"   texcoords.x = texcoords.x/2.0;\n"
"}";

static const char* std_shader = \
"#version 150\n"
"in vec2 texcoords;\n"
"uniform sampler2D texture_in;\n"
"uniform vec2 origin;\n"
"uniform vec2 ratio;\n"
"out vec4 frag_colour;\n"
"void main()\n"
"{\n"
"   vec2 coord = vec2( texcoords.x *ratio.x + origin.x, texcoords.y*ratio.y + origin.y);\n"
"   frag_colour = texture(texture_in, coord);\n"
"}\n";

CDisplay::CDisplay() : current_texture_(0), current_index_of_index_to_display_(0), number_of_frame_to_display_(0), fragment_shader_(0), program_(0)
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
   GLsizei sizelog;

   for (int i = 0; i < NB_FRAMES; i++)
   {
      framebufferArray_[i] = new int[1024 * 1024];
   }

   ///////////////////////
   // Init shaders
   fragment_shader_ = glCreateShader(GL_FRAGMENT_SHADER);
   glShaderSource(fragment_shader_, 1, &std_shader, NULL);
   glCompileShader(fragment_shader_);
   glGetShaderInfoLog(fragment_shader_, 256, &sizelog, log);

   vertex_shader_ = glCreateShader(GL_VERTEX_SHADER);
   glShaderSource(vertex_shader_, 1, &vertex_quad_shader_2, NULL);
   glCompileShader(vertex_shader_);
   glGetShaderInfoLog(vertex_shader_, 256, &sizelog, log);

   program_ = glCreateProgram();
   glAttachShader(program_, vertex_shader_);
   glAttachShader(program_, fragment_shader_);
   glLinkProgram(program_);
   GLint status;

   ///////////////////////
   // Init vertices
static float vertices[] = { -1.0, -1.0,   1.0, -1.0,   1.0, 1.0,    // Triangle 1
                           -1.0, -1.0,   -1.0, 1.0,   1.0, 1.0 };   // Triangle 2
/*   static float vertices[] = { -1.0, -0.8,   1.0, -0.8,   1.0, 1.2,    // Triangle 1
                             -1.0, -0.8,   -1.0, 1.2,   1.0, 1.2 };   // Triangle 2*/

   int tailleVerticesBytes = 12 * sizeof(float);
   glGenBuffers(1, &vbo);
   glBindBuffer(GL_ARRAY_BUFFER, vbo);
   glBufferData(GL_ARRAY_BUFFER, tailleVerticesBytes, 0, GL_STATIC_DRAW);
   glBufferSubData(GL_ARRAY_BUFFER, 0, tailleVerticesBytes, vertices);
   glBindBuffer(GL_ARRAY_BUFFER, 0);

   // VAO
   glGenVertexArrays(1, &vao);
   glBindVertexArray(vao);
   glBindBuffer(GL_ARRAY_BUFFER, vbo);
   glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)(0));
   glEnableVertexAttribArray(0);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindVertexArray(0);

   glUseProgram(program_);

   //sh_texture_ = glGetUniformLocation(program_, "texture_in");
   glGenTextures(NB_FRAMES, texture_);
   glBindTexture(GL_TEXTURE_2D, texture_[0]);

   // Setup filtering parameters for display
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1024, 1024, 0, GL_RGBA, GL_UNSIGNED_BYTE, framebufferArray_[0]);

   sh_origin_ = glGetUniformLocation(program_, "origin");
   sh_ratio_ = glGetUniformLocation(program_, "ratio");

   float ratiox = (float)DISP_WINDOW_X / (float)REAL_DISP_X;
   float ratioy = (float)DISP_WINDOW_Y / (float)REAL_DISP_X;

   glUniform2f(sh_ratio_, ratiox, ratioy );
   glUniform2f(sh_origin_, ORIGIN_X / 1024 * ratiox, ORIGIN_Y / 1024 * ratioy);
   
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

GLuint CDisplay::GetTexture()
{
   return texture_[0];/* current_index_of_index_to_display_*/;
}

void CDisplay::Display()
{
   if (number_of_frame_to_display_ > 0)
   {

      glUseProgram(program_);
      glBindVertexArray(vao);

      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, texture_[0]);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1024, 1024, 0, GL_BGRA, GL_UNSIGNED_BYTE, framebufferArray_[current_index_of_index_to_display_]);

      // Rendu
      glDrawArrays(GL_TRIANGLES, 0, 6);
      // Déverrouillage du VAO
      glBindVertexArray(0);

      glUseProgram(0);
   }

}

void CDisplay::WaitVbl ()
{
}
