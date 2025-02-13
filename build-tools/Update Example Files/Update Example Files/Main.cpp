#include "stdafx.h"
#include "ExampleFileUpdater.h"


int wmain(int argc, wchar_t* argv[])
{
    bool error = false;

    try
    {
        // read the examples directory...
        std::wstring examples_directory;
    
        // ...from the command line
        if( argc >= 2 )
        {
            examples_directory = argv[1];
        }

        // ...or calculate it based on an assumed directory structure of code/examples and cspro/...
        else
        {
            std::wstring application_directory(_MAX_PATH, '\0');
            GetModuleFileName(nullptr, application_directory.data(), _MAX_PATH);
            application_directory.resize(_tcslen(application_directory.data()));
            application_directory = PortableFunctions::PathGetDirectory(application_directory);

            examples_directory = MakeFullPath(application_directory, _T("..\\..\\..\\..\\code\\examples"));
        }

        if( !PortableFunctions::FileIsDirectory(examples_directory) )
            throw CSProException(_T("The directory does not exist: %s"), examples_directory.c_str());

        ExampleFileUpdater().Update(examples_directory);
    }

    catch( const CSProException& exception )
    {
        std::wcout << _T("\n\nFailure\n-------\n") << exception.GetErrorMessage();
        error = true;
    }

    std::wcout << _T("\n\n");

    return error ? -1 : 0;
}
