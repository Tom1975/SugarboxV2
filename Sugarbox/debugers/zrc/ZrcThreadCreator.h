#include "ZrcDebugThread.h"
#include "../IThreadCreator.h"

class ZrcThreadCreator : public IThreadCreator
{
public:
    virtual QThread* CreateThread(Emulation* emulation, qintptr socketDescriptor, DebugSocket* socket)
    {
        return new ZrcDebugThread(emulation, socketDescriptor, socket);
    }
};
