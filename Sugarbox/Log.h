#pragma once

#include "ILog.h"

class Log : public ILog
{
public:

   virtual void WriteLog(const char* pLog);
   virtual void WriteLogByte(unsigned char pNumber);
   virtual void WriteLogShort(unsigned short pNumber);
   virtual void WriteLog(unsigned int pNumber);
   virtual void EndOfLine();

};

