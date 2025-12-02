#include "stdafx.h"
#include "Main.h"


int wmain(const int argc, const wchar_t* const argv[])
{
    try
    {
        const std::string command = ( argc >= 2 ) ? TC::ToUtf8(argv[1]) : 
                                                    std::string();

        ( command == "audit" ) ? MessageFileAuditor().DoAudit() :
        ( command == "assets") ? AssetsGenerator::Create() :
        ( command == "format") ? MessageFormatter().FormatMessageFiles() : 
                                 throw CSProException("Run this program with the command line argument \"audit\" or \"assets\" or \"format\"");
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }

    return 0;
}
