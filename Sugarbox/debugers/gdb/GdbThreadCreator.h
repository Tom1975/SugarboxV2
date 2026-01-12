#include "GdbDebugThread.h"
#include "../IThreadCreator.h"

class GdbThreadCreator : public IThreadCreator
{
public:
    virtual QThread* CreateThread(Emulation* emulation, qintptr socketDescriptor, DebugSocket* socket)
    {
        return new GdbDebugThread(emulation, socketDescriptor, socket);
    }
};
