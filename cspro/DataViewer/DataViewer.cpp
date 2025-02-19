#include "DataViewer.h"
#include <zToolsO/Tools.h>
#include <zToolsO/Utf8.h>
#include <zUtilO/ConnectionString.h>
#include <zUtilO/CSProExecutables.h>
#include <zUtilO/Versioning.h>
#include <zDataO/ConnectionStringProperties.h>


// Data Viewer is being kept around, temporarily, to forward command line arguments to Data Manager
static_assert(Versioning::Number <= 8.1, "remove Data Viewer from the solution, installer, and list of executables in the readme");


// The one and only DataViewerApp object
DataViewerApp theApp;


DataViewerApp::DataViewerApp()
{
    InitializeCSProEnvironment();
}


BOOL DataViewerApp::InitInstance()
{
    // command line arguments can come in a variety of ways
    // 1) [PFF file path]
    // 2) [data file path] [optional dictionary file path]
    // 3) [file=data file path] [dcf=optional dictionary file path] [key=case key to select] [uuid=optional case uuid to select]

    std::string file_path;
    std::string dictionary_file_path;
    std::string case_key;
    std::string case_uuid;

    std::vector<std::string> arguments;

    for( int i = 1; i < __argc; ++i )
    {
        const wchar_t* const arg = __wargv[i];
        ASSERT(arg != nullptr);

        // ignore flags
        if( *arg == '-' || *arg == '/' )
            continue;

        const std::string& argument = arguments.emplace_back(TC::ToUtf8(arg));

        const auto [command_sv, value_sv] = SO::GetTextOnEitherSideOfCharacter(argument, '=');

        if( value_sv.empty() )
            continue;

        if( command_sv == "file" )
        {
            file_path = MakeFullPath(GetWorkingDirectory(), std::string(value_sv));
        }

        else if( command_sv == "dcf" )
        {
            dictionary_file_path = MakeFullPath(GetWorkingDirectory(), std::string(value_sv));
        }

        else if( command_sv == "key" )
        {
            case_key = value_sv;
        }

        else if( command_sv == "uuid" )
        {
            case_uuid = value_sv;
        }

        else
        {
            break;
        }
    }

    // if not using the command/value approach, the file path will be the first argument
    // and the dictionary will be the second optional argument
    if( file_path.empty() && !arguments.empty() )
    {
        file_path = MakeFullPath(GetWorkingDirectory(), arguments.front());

        if( dictionary_file_path.empty() && arguments.size() > 1 )
            dictionary_file_path = MakeFullPath(GetWorkingDirectory(), arguments[1]);
    }

    // forward the arguments to Data Manager
    try
    {
        if( file_path.empty() )
        {
            CSProExecutables::RunProgram(CSProExecutables::Program::DataManager, nullptr, true);
        }

        else
        {
            ConnectionString connection_string(file_path);

            if( !dictionary_file_path.empty() )
                connection_string.SetProperty(CSProperty::dictionaryPath, dictionary_file_path);

            if( !case_key.empty() )
                connection_string.SetProperty(CSProperty::key, case_key);

            if( !case_uuid.empty() )
                connection_string.SetProperty(CSProperty::uuid, case_uuid);

            CSProExecutables::RunProgramOpeningFile(CSProExecutables::Program::DataManager, connection_string.ToString(), true);
        }
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }

    return FALSE;
}
