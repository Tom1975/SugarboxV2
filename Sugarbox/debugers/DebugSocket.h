#pragma once

#include <functional>

#include <QtWebSockets/QtWebSockets>
#include <QTcpServer>
#include <QTcpSocket>
#include "Emulation.h"
#include "IThreadCreator.h"
#include "DebugCommand.h"

class DebugSocket : public QTcpServer
{
   Q_OBJECT
public:
   explicit DebugSocket(QObject* parent, Emulation* emulation, IThreadCreator* creator, unsigned short port);
   void StartServer();

signals:

public slots:

protected:   
   void incomingConnection(qintptr socketDescriptor);
protected:
   IThreadCreator* creator_;
   Emulation* emulation_;
   unsigned short port_;
};

