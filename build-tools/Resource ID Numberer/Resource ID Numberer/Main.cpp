#include "stdafx.h"
#include "Numberer.h"


int wmain(const int argc, const wchar_t* const argv[])
{
    try
    {
        std::string definitions_file_path;
        bool only_process_recent_changes = false;

        for( int i = 1; i < argc; ++i )
        {
            if( i > 2 )
                throw CSProException("More than 2 arguments are not allowed");

            std::string argument = TC::ToUtf8(argv[i]);

            if( !only_process_recent_changes && argument == "/recent" )
            {
                only_process_recent_changes = true;
            }

            else if( definitions_file_path.empty() )
            {
                definitions_file_path = MakeFullPath(GetWorkingDirectory(), std::move(argument));
            }

            else
            {
                throw CSProException("Unknown argument #%d: %s", i, argument.c_str());
            }
        }

        if( definitions_file_path.empty() )
            throw CSProException("Specify the name of the file with the resource ID definitions.");

        Numberer numberer(definitions_file_path, only_process_recent_changes);
        numberer.Run();
    }

    catch( const CSProException& exception )
    {
        MessageBoxW(nullptr, TC::ToWide(exception.what()).c_str(), L"Resource ID Numberer", MB_OK | MB_ICONEXCLAMATION);
    }
}
