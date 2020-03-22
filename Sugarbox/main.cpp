#ifdef WIN32
   #pragma comment(linker, " /ENTRY:mainCRTStartup")
#endif

#include "SugarboxApp.h"

int main()
{
   SugarboxApp app;
   return app.RunApp();
}

