#pragma once

#include <QtWebSockets/QtWebSockets>
#include <QTcpServer>
#include <QTcpSocket>
#include "Emulation.h"

class DebugSocket : public QObject
{
   Q_OBJECT
public:
   DebugSocket(QObject * parent, Emulation* emulation);
   virtual~DebugSocket();

signals:

public slots:
   void newConnection();

protected:
   Emulation* emulation_;
   QTcpServer  *server;
};

