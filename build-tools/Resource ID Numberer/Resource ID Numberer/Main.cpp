#include "stdafx.h"
#include "Numberer.h"


int wmain(int argc, wchar_t* argv[])
{
    try
    {
        std::wstring definitions_filename;
        bool only_process_recent_changes = false;

        for( int i = 1; i < argc; ++i )
        {
            if( i > 2 )
            {
                throw CSProException("More than 2 arguments are not allowed");
            }

            else if( !only_process_recent_changes && SO::Equals(argv[i], "/recent") )
            {
                only_process_recent_changes = true;
            }

            else if( definitions_filename.empty() )
            {
                definitions_filename = MakeFullPath(GetWorkingFolder(), argv[i]);
            }

            else
            {
                throw CSProException(L"Unknown argument #%d: %s", i, argv[i]);
            }
        }

        if( definitions_filename.empty() )
            throw CSProException("Specify the name of the file with the resource ID definitions.");

        Numberer numberer(definitions_filename, only_process_recent_changes);
        numberer.Run();
    }

    catch( const CSProException& exception )
    {
        MessageBoxW(nullptr, exception.GetErrorMessage().c_str(), L"Resource ID Numberer", MB_OK | MB_ICONEXCLAMATION);
    }
}
