#include "Log.h"

#include <cstdio>

#ifdef _WIN32
#include <windows.h>
#endif

// Shared by every frontend : no dependency on the UI toolkit. The text goes where qDebug sent it :
// the debugger output on Windows, the error output elsewhere.
static void Output(const char* text)
{
#ifdef _WIN32
   OutputDebugStringA(text);
#else
   std::fputs(text, stderr);
#endif
}

void Log::WriteLog(const char* pLog)
{
   Output(pLog);
   Output("\n");
}

void Log::WriteLogByte(unsigned char pNumber)
{
   char text[8];
   std::snprintf(text, sizeof(text), " %2.2x", pNumber);
   Output(text);
}

void Log::WriteLogShort(unsigned short pNumber)
{
   char text[8];
   std::snprintf(text, sizeof(text), " %4.4x", pNumber);
   Output(text);
}

void Log::WriteLog(unsigned int pNumber)
{
   char text[16];
   std::snprintf(text, sizeof(text), " %8.8x", pNumber);
   Output(text);
}

void Log::EndOfLine()
{
   Output("\n");
}
