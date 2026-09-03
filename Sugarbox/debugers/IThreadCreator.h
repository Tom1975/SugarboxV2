#pragma once 

#include <QtWebSockets/QtWebSockets>
#include "Emulation.h"

class DebugSocket;

class IThreadCreator
{
public:
    virtual QThread* CreateThread(Emulation* emulation, qintptr socketDescriptor, DebugSocket*) = 0;
};

