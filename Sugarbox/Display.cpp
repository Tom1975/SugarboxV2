
#include "Display.h"

#define REAL_DISP_X  1024 //832 //1024 // 768
#define REAL_DISP_Y  624 //-16 //624 //576

#define DISP_WIDTH    1024
#define DISP_HEIGHT   624

#define DISP_WINDOW_X   800
#define DISP_WINDOW_Y   600

static const char* vertex_quad_shader = \
"#version 410\n"\
"layout(location = 0) out vec2 uv;\n"\
"void main()\n"\
"{\n"\
"   float x = float(((uint(gl_VertexID) + 2u) / 3u) % 2u);\n"\
"   float y = float(((uint(gl_VertexID) + 1u) / 3u) % 2u);\n"\
"   gl_Position = vec4(-1.0f + x * 2.0f, -1.0f + y * 2.0f, 0.0f, 1.0f);\n"\
"   uv = vec2(x, y);\n"\
"}\n";

static const char* vertex_quad_shader_2 = \
"#version 130\n"
"in vec2 in_Vertex;"
"out vec2 texcoords;"
"void main() {"
/*"   float x = float(((uint(gl_VertexID) + 2u) / 3u) % 2u);\n"
"   float y = float(((uint(gl_VertexID) + 1u) / 3u) % 2u);\n"
"   gl_Position = vec4(-1.0f + x * 2.0f, -2.0f + y * 1.0f, 0.0f, 1.0f);\n"*/
"   gl_Position = vec4(in_Vertex, 0.0f, 1.0f);\n"
"   texcoords = in_Vertex;\n"
"}";

static const char* std_shader = \
//"#version 150 core\n"
"#version 130\n"
"in vec2 texcoords;\n"
"uniform sampler2D texture_in;\n"
/*"uniform vec2 origin;\n"
"uniform vec2 size_of_display;\n"
"uniform vec2 size_of_texture;\n"*/
"out vec4 frag_colour;\n"
"void main()\n"
"{\n"
"   frag_colour = texture(texture_in, texcoords);\n"
/*"   vec2 coord = vec2((texcoords.x * size_of_display.x + origin.x) / size_of_texture.x, ((1.0-texcoords.y)/2 * size_of_display.y + origin.y) / size_of_texture.y);\n"*/
/*"   vec2 coord = vec2((texcoords.x ) , ((1.0-texcoords.y)/2 ) );\n"*/
//"   frag_colour = vec4(texcoords.x, texcoords.y, 1.0f, 1.0f);\n"
//"     frag_colour = vec4(0.5f, 1.0f , 1.0f, 1.0f); \n"
"}\n";

/*"#version 400\n"
"in vec3 texcoords;\n"
"uniform vec4 inputColour;\n"
"uniform sampler2D texture_in;\n"
"uniform vec2 origin;\n"
"uniform vec2 size_of_display;\n"
"uniform vec2 size_of_texture;\n"
"out vec4 frag_colour;\n"
"void main()\n"
"{\n"
"   vec2 coord = vec2((texcoords.x * size_of_display.x + origin.x) / size_of_texture.x, ((1.0-texcoords.y) * size_of_display.y + origin.y) / size_of_texture.y);\n"
"   vec4 pixel = texture(texture_in, texcoords);\n"
"   pixel = vec4( pixel.r, pixel.g, pixel.b,  1.0f);"
"   frag_colour =  inputColour * pixel;\n"
"}\n"
"";*/

/*
static const vec2 vertices[4] =
{
    { 0.f, 0.f },
    { 1.f, 0.f },
    { 1.f, 1.f },
    { 0.f, 1.f }
};*/

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
   //return  (0xFF000000 | ((rgb & 0xFF)<<16) | ((rgb & 0xFF00) ) | ((rgb & 0xFF0000)>>16));
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

   // Init fragment shader
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


   static float vertices[] = { -1.0, -1.0,   1.0, -1.0,   1.0, 1.0,    // Triangle 1
                           -1.0, -1.0,   -1.0, 1.0,   1.0, 1.0 };   // Triangle 2

   int tailleVerticesBytes = 12 * sizeof(float);
   // Génération du 
   glGenBuffers(1, &vbo);
   // Verrouillage
   glBindBuffer(GL_ARRAY_BUFFER, vbo);
   // Allocation
   glBufferData(GL_ARRAY_BUFFER, tailleVerticesBytes, 0, GL_STATIC_DRAW);
   glBufferSubData(GL_ARRAY_BUFFER, 0, tailleVerticesBytes, vertices);
   // Déverrouillage
   glBindBuffer(GL_ARRAY_BUFFER, 0);

   // VAO
   // Génération du VAO
   glGenVertexArrays(1, &vao);
   glBindVertexArray(vao);
   glBindBuffer(GL_ARRAY_BUFFER, vbo);

   // Vertex Attrib 0 (Vertices)
   glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)(0));
   glEnableVertexAttribArray(0);

   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindVertexArray(0);



   sh_texture_ = glGetUniformLocation(program_, "texture_in");
   glGenTextures(NB_FRAMES, texture_);
   glBindTexture(GL_TEXTURE_2D, texture_[0]);

   // Setup filtering parameters for display
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1024, 1024, 0, GL_RGBA, GL_UNSIGNED_BYTE, framebufferArray_[0]);


   /*char log[256];
   GLsizei sizelog;

   static const GLfloat g_vertex_buffer_data[] = {
      -1.0f, -1.0f, 0.0f,
      1.0f, -1.0f, 0.0f,
      1.0f,  1.0f, 0.0f,
      -1.0f,  1.0f, 0.0f,
   };
   // VAO init
   glGenVertexArrays(1, &VertexArrayID_);
   glBindVertexArray(VertexArrayID_);

   // This will identify our vertex buffer
   // Generate 1 buffer, put the resulting identifier in vertexbuffer
   glGenBuffers(1, &vertexbuffer_);
   // The following commands will talk about our 'vertexbuffer' buffer
   glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer_);
   // Give our vertices to OpenGL.
   glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);


   // Init fragment shader
   fragment_shader_ = glCreateShader(GL_FRAGMENT_SHADER);
   glShaderSource(fragment_shader_, 1, &std_shader, NULL);
   glCompileShader(fragment_shader_);
   glGetShaderInfoLog(fragment_shader_, 256, &sizelog, log);

   vertex_shader_ = glCreateShader(GL_VERTEX_SHADER);
   glShaderSource(vertex_shader_, 1, &vertex_quad_shader, NULL);
   glCompileShader(vertex_shader_);
   glGetShaderInfoLog(vertex_shader_, 256, &sizelog, log);

   program_ = glCreateProgram();
   glAttachShader(program_, vertex_shader_);
   glAttachShader(program_, fragment_shader_);
   glLinkProgram(program_);
   GLint status;
   glUseProgram(program_);


   glGetProgramiv(program_, GL_LINK_STATUS, &status);
   glGetProgramInfoLog(program_, 256, &sizelog, log);

   sh_texture_ = glGetUniformLocation(program_, "texture_in");
   sh_origin_ = glGetUniformLocation(program_, "origin");
   sh_size_of_display_ = glGetUniformLocation(program_, "size_of_display");
   sh_size_of_texture_ = glGetUniformLocation(program_, "size_of_texture");

   glUniform2f(sh_origin_, 143.f, 23.f / 2);
   glUniform2f(sh_size_of_display_, 768.f, 576.f / 2);
   glUniform2f(sh_size_of_texture_, REAL_DISP_X, REAL_DISP_Y);
   glGetProgramInfoLog(program_, 256, &sizelog, log);

   sh_texture_ = glGetUniformLocation(program_, "texture_in");
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

   readFboId_ = 0;
   glGenFramebuffers(1, &readFboId_);
   glBindFramebuffer(GL_READ_FRAMEBUFFER, readFboId_);
   glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
      GL_TEXTURE_2D, texture_[0], 0);
   glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
   //Reset();
   */
}

void CDisplay::Init40 ()
{
   char log[256];
   GLsizei sizelog;
   
   static const GLfloat g_vertex_buffer_data[] = {
      -1.0f, -1.0f, 0.0f,
      1.0f, -1.0f, 0.0f,
      1.0f,  1.0f, 0.0f,
      1.0f,  1.0f, 0.0f,
      -1.0f, -1.0f, 0.0f,
      -1.0f,  1.0f, 0.0f,
   };
   // VAO init
   glGenVertexArrays(1, &VertexArrayID_);
   glBindVertexArray(VertexArrayID_);

   // This will identify our vertex buffer
   // Generate 1 buffer, put the resulting identifier in vertexbuffer
   glGenBuffers(1, &vertexbuffer_);
   // The following commands will talk about our 'vertexbuffer' buffer
   glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer_);
   // Give our vertices to OpenGL.
   glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

   for (int i = 0; i < NB_FRAMES; i++)
   {
      framebufferArray_[i] = new int[1024 * 1024];
   }


   // Init fragment shader
   fragment_shader_ = glCreateShader(GL_FRAGMENT_SHADER);
   glShaderSource(fragment_shader_, 1, &std_shader, NULL);
   glCompileShader(fragment_shader_);
   glGetShaderInfoLog(fragment_shader_, 256, &sizelog, log);

   vertex_shader_ = glCreateShader(GL_VERTEX_SHADER);
   glShaderSource(vertex_shader_, 1, &vertex_quad_shader, NULL);
   glCompileShader(vertex_shader_);
   glGetShaderInfoLog(vertex_shader_, 256, &sizelog, log);

   program_ = glCreateProgram();
   glAttachShader(program_, vertex_shader_);
   glAttachShader(program_, fragment_shader_);
   glLinkProgram(program_);
   GLint status;
   glUseProgram(program_);


   glGetProgramiv(program_, GL_LINK_STATUS, &status);
   glGetProgramInfoLog(program_, 256, &sizelog, log);

   sh_texture_ = glGetUniformLocation(program_, "texture_in");
   sh_origin_ = glGetUniformLocation(program_, "origin");
   sh_size_of_display_ = glGetUniformLocation(program_, "size_of_display");
   sh_size_of_texture_ = glGetUniformLocation(program_, "size_of_texture");

   glUniform2f(sh_origin_, 143.f, 23.f/2);
   glUniform2f(sh_size_of_display_, 768.f, 576.f / 2);
   glUniform2f(sh_size_of_texture_, REAL_DISP_X, REAL_DISP_Y);
   glGetProgramInfoLog(program_, 256, &sizelog, log);

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

   readFboId_ = 0;
   glGenFramebuffers(1, &readFboId_);
   glBindFramebuffer(GL_READ_FRAMEBUFFER, readFboId_);
   glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
      GL_TEXTURE_2D, texture_[0], 0);
   glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
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
// VBO et taille des données
// Verrouillage du VAO
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

      /*
      glEnableVertexAttribArray(0);
      glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer_);
      glVertexAttribPointer(
         0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
         3,                  // size
         GL_FLOAT,           // type
         GL_FALSE,           // normalized?
         0,                  // stride
         (void*)0            // array buffer offset
      );
      // Draw the triangle !
      glUseProgram(program_);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, texture_[0]);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1024, 1024, 0, GL_BGRA, GL_UNSIGNED_BYTE, framebufferArray_[current_index_of_index_to_display_]);
      glDrawArrays(GL_TRIANGLES, 0, 6); // Starting from vertex 0; 3 vertices total -> 1 triangle
      glDisableVertexAttribArray(0);
      */


      /*
      glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer_);
      glUseProgram(program_);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, texture_[0]);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1024, 1024, 0, GL_BGRA, GL_UNSIGNED_BYTE, framebufferArray_[current_index_of_index_to_display_]);

      glDrawArrays(GL_TRIANGLES, 0, 6); // Starting from vertex 0; 3 vertices total -> 1 triangle
      */
   }

}

void CDisplay::WaitVbl ()
{
}
