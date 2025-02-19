#include "stdafx.h"
#include "ExampleFileUpdater.h"


int wmain(const int argc, const wchar_t* const argv[])
{
    bool error = false;

    try
    {
        // read the examples directory...
        std::string examples_directory;
    
        // ...from the command line
        if( argc >= 2 )
        {
            examples_directory = TC::ToUtf8(argv[1]);
        }

        // ...or calculate it based on an assumed directory structure of code/examples and cspro/...
        else
        {
            auto application_directory = std::make_unique_for_overwrite<wchar_t[]>(_MAX_PATH);
            GetModuleFileName(nullptr, application_directory.get(), _MAX_PATH);

            examples_directory = MakeFullPath(PortableFunctions::PathGetDirectory(TC::ToUtf8(application_directory.get())), 
                                              "..\\..\\..\\..\\code\\examples");
        }

        if( !PortableFunctions::FileIsDirectory(examples_directory) )
            throw CSProException("The directory does not exist: %s", examples_directory.c_str());

        ExampleFileUpdater().Update(examples_directory);
    }

    catch( const CSProException& exception )
    {
        std::wcout << L"\n\nFailure\n-------\n" << TC::ToWide(exception.what());
        error = true;
    }

    std::wcout << L"\n\n";

    return error ? -1 : 0;
}
