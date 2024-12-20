#pragma once

// C RunTime Header Files
#include <stdlib.h>
#include <string>
#include <memory.h>
#include <mutex>

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLBuffer>

#include "Screen.h"

QT_FORWARD_DECLARE_CLASS(QOpenGLShaderProgram);
QT_FORWARD_DECLARE_CLASS(QOpenGLTexture)

#define NB_FRAMES 3

// Display
class CDisplay : public QOpenGLWidget, protected QOpenGLFunctions, public IDisplay
{
   Q_OBJECT

public :
   explicit CDisplay(QWidget *parent = 0);
   virtual ~CDisplay ();

   void Init();
   void Show(bool bShow);

   void SyncOnFrame(bool set) override ;
   bool IsSyncOnFrame() { return sync_on_frame_; }

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
   virtual void Screenshot (const char* scr_path);
   virtual void Screenshot();
   virtual void ScreenshotEveryFrame(int bSetOn) {};
   virtual bool IsEveryFrameScreened() {
      return false;
   }
   virtual bool IsDisplayed() { return true;/* m_bShow;*/ };
   virtual void FullScreenToggle (){};
   virtual void ForceFullScreen (bool bSetFullScreen ){}
   virtual void WindowChanged (int xIn, int yIn, int wndWidth, int wndHeight){};
   virtual bool SetSyncWithVbl ( int speed ){return false; };
   virtual bool IsWaitHandled();
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

signals:
   void FrameIsReady();

public slots:
   void Display();

protected:
   void initializeGL() override;
   void paintGL() override;

protected:

   // Displayed window : 
   int frame_emitted_;
   int m_X, m_Y;
   int m_Width;
   int m_Height;

   class FrameItem
   {
   public:
      FrameItem() {
         framebufferArray_ = new int[1024 * 1024];
         sample_number_ = 0;
      }
      virtual ~FrameItem() {
         delete[]framebufferArray_;
      }

      void Init() {
         status_ = FREE;
         sample_number_ = 0;   
      }

      enum BufferState {
         FREE,
         IN_USE,
         LOCKED,
         TO_PLAY,
      };

      int* framebufferArray_;
      BufferState status_;
      int sample_number_;
   };

   FrameItem* buffer_list_;
   int sample_number_;

   // Current buffer lists
   int index_current_buffer_;
   // Window
   //int* framebufferArray_[NB_FRAMES];

   // Textures to display indexes
   int index_to_display_[NB_FRAMES];
   int current_index_of_index_to_display_;
   int number_of_frame_to_display_;

   // Texture Indexes
   int current_texture_;

   bool sync_on_frame_;

   // Open gl stuff
   QColor clearColor;
   QPoint lastPos;
   int xRot;
   int yRot;
   int zRot;
   QOpenGLTexture *textures[1];
   QOpenGLShaderProgram *program;
   QOpenGLBuffer vbo;

   std::mutex sync_mutex_;
};
