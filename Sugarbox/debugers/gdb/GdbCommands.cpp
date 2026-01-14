#include <QtCore>

#include "GdbCommands.h"

////////////////////////////////////////////////////////
/// query 'q'
bool RemoteCommandQuery::Execute(std::vector<std::string>& commandlist)
{
    if ( commandlist.size() == 0 )
    {
        qDebug () << "query command : " << QString::fromStdString (commandlist[0]);
        callback_->SendResponse("E01");
        return false;
    }

    qDebug () << "query command : " << QString::fromStdString (commandlist[0]);

    // TODO : This is cheat. Need a correct analysis / answer
    callback_->SendResponse("PacketSize=4000;xmlRegisters=target");

    return true;
}

std::string RemoteCommandQuery::Help()
{
    return "query";
}