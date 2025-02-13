#include "stdafx.h"
#include "Main.h"


int wmain(int argc, wchar_t* argv[])
{
    try
    {
        const std::wstring command = ( argc >= 2 ) ? argv[1] : std::wstring();

        SO::Equals(command, L"audit")  ? MessageFileAuditor().DoAudit() :
        SO::Equals(command, L"assets") ? AssetsGenerator::Create() :
        SO::Equals(command, L"format") ? MessageFormatter().FormatMessageFiles() : 
                                         throw CSProException("Run this program with the command line argument \"audit\" or \"assets\" or \"format\"");
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }

    return 0;
}
