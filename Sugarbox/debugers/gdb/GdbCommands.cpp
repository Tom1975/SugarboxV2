#include <QtCore>

#include "GdbCommands.h"

////////////////////////////////////////////////////////
/// XML for z80
std::string xml_z80 = "<?xml version=\"1.0\"?> \
<!DOCTYPE target SYSTEM \"gdb-target.dtd\"> \
<target>\
    <architecture>i8086</architecture>\
    <feature name=\"org.gnu.gdb.i8086\">\
    <reg name=\"af\" bitsize=\"16\"/>\
    <reg name=\"bc\" bitsize=\"16\"/>\
    <reg name=\"de\" bitsize=\"16\"/>\
    <reg name=\"hl\" bitsize=\"16\"/>\
    <reg name=\"sp\" bitsize=\"16\"/>\
    <reg name=\"pc\" bitsize=\"16\"/>\
  </feature>\
</target>";

////////////////////////////////////////////////////////
/// query 'q'

RemoteCommandQuery::RemoteCommandQuery():IRemoteCommand()
{
}

bool RemoteCommandQuery::Execute(std::vector<std::string>& commandlist)
{
    if ( commandlist.size() == 0 )
    {
        qDebug () << "query command is empty";
        callback_->SendResponse("E01");
        return false;
    }

    if ( commandlist[0].rfind( "Supported", 0) == 0)
    {
        callback_->SendResponse("PacketSize=4000;qXfer:features:read+");
    }
    else if ( commandlist[0].rfind( "Attached", 0) == 0)
    {
        callback_->SendResponse("T05");
    }
    else if ( commandlist[0].rfind( "TStatus", 0) == 0)
    {
        callback_->SendResponse("");
    }
    else if ( commandlist[0].rfind( "fThreadInfo", 0) == 0)
    {
        callback_->SendResponse("m1");
    } 
    else if ( commandlist[0].rfind( "sThreadInfo", 0) == 0)
    {
        callback_->SendResponse("l");
    }    
    // current thread
    else if ( commandlist[0].rfind( "C", 0) == 0)
    {
        callback_->SendResponse("QC1");
    }    
    else if ( commandlist[0].rfind( "Xfer", 0) == 0)
    {
        return HandleTransfert( commandlist[0]); 
    }
    else
    {
        callback_->SendResponse("E01");
    }

    

    return true;
}

std::vector<std::string> split(std::string& s, const std::string& delimiter) 
{
    std::vector<std::string> tokens;
    size_t pos = 0;
    std::string token;
    while ((pos = s.find(delimiter)) != std::string::npos) {
        token = s.substr(0, pos);
        tokens.push_back(token);
        s.erase(0, pos + delimiter.length());
    }
    tokens.push_back(s);

    return tokens;
}

bool RemoteCommandQuery::HandleTransfert (std::string command)
{
    // Decode command
    auto arg = split (command, ":");

    if (true)
        {
            // Check for something like 0,100
            for (auto it:  arg)
            {
                int b, f;
                if (sscanf(it.c_str(), "%i,%i", &b,&f) == 2)
                {
                    // return extract from target.xml
                    if ( b > xml_z80.size())
                    {
                        callback_->SendResponse("l");
                        return true;
                    }

                    std::string buffer_to_send = (b+f >= xml_z80.size())?"l":"m";
                    if ( b + f > xml_z80.size())
                    {
                        f = xml_z80.size() - b;
                    }

                    buffer_to_send += xml_z80.substr(b, f);
                    callback_->SendResponse(buffer_to_send.c_str());
                    return true;
                }
            }
        }
    else
    {
        callback_->SendResponse("E01");
    }

    return true;
}

std::string RemoteCommandQuery::Help()
{
    return "query";
}

////////////////////////////////////////////////////////
/// query 'v'

RemoteCommandV::RemoteCommandV():IRemoteCommand()
{
}

bool RemoteCommandV::Execute(std::vector<std::string>& commandlist)
{
    if ( commandlist[0].rfind( "Cont?", 0) == 0)
    {
        callback_->SendResponse("vCont;c;s");
    }
    else if ( commandlist[0].rfind( "MustReplyEmpty", 0) == 0)
    {
        callback_->SendResponse("");
    }

    return true;
}

std::string RemoteCommandV::Help()
{
    return "vCont";
}

////////////////////////////////////////////////////////
/// query 'H'

RemoteCommandH::RemoteCommandH():IRemoteCommand()
{
}

bool RemoteCommandH::Execute(std::vector<std::string>& commandlist)
{
    if ( commandlist[0].rfind( "g0", 0) == 0)
    {
        callback_->SendResponse("OK");
    } 
    else if ( commandlist[0].rfind( "c-1", 0) == 0)
    {
        callback_->SendResponse("OK");
    }
    
    return true;
}

std::string RemoteCommandH::Help()
{
    return "H";
}

////////////////////////////////////////////////////////
/// query 'C'

RemoteCommandC::RemoteCommandC():IRemoteCommand()
{
}

bool RemoteCommandC::Execute(std::vector<std::string>& commandlist)
{
    if ( commandlist[0].rfind( "-1", 0) == 0)
    {
        // Run CPU ?
        callback_->SendResponse("S05");
    } 
    
    return true;
}

std::string RemoteCommandC::Help()
{
    return "C";
}



////////////////////////////////////////////////////////
/// query '?'
RemoteCommandAsk::RemoteCommandAsk():IRemoteCommand()
{
}

bool RemoteCommandAsk::Execute(std::vector<std::string>& commandlist)
{
    if ( commandlist[0].size() == 0)
    {
        // Run CPU ?
        callback_->SendResponse("S05");
    } 
    
    return true;
}

std::string RemoteCommandAsk::Help()
{
    return "?";
}


////////////////////////////////////////////////////////
/// query 'g'
RemoteCommandStack::RemoteCommandStack():IRemoteCommand()
{
}

bool RemoteCommandStack::Execute(std::vector<std::string>& commandlist)
{
    // Run CPU ?
    callback_->SendResponse("00112233445566778899aabb");
    
    return true;
}

std::string RemoteCommandStack::Help()
{
    return "?";
}
