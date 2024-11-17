#include "Log.h"
#include <QDebug>

void Log::WriteLog(const char* pLog)
{  
   qDebug() << pLog;
};

void Log::WriteLogByte(unsigned char pNumber)
{
   qDebug() << QString(" %1").arg(pNumber, 2, 16, QChar('0'));
}

void Log::WriteLogShort(unsigned short pNumber) 
{
   qDebug() << QString(" %1").arg(pNumber, 4, 16, QChar('0'));
}

void Log::WriteLog(unsigned int pNumber) 
{
   qDebug() << QString(" %1").arg(pNumber, 8, 16, QChar('0'));
}

void Log::EndOfLine()
{ 
   qDebug().noquote() << "\n";
}
