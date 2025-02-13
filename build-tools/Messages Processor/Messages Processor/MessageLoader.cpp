#include "stdafx.h"
#include "Main.h"
#include <zToolsO/DirectoryLister.h>
#include <zToolsO/Tools.h>
#include <zToolsO/VectorHelpers.h>


std::wstring MessageLoader::GetCSProDevelopmentDirectory()
{
    std::wstring application_directory(_MAX_PATH, '\0');
    GetModuleFileName(nullptr, application_directory.data(), application_directory.length());
    application_directory.resize(_tcslen(application_directory.data()));

    application_directory = PortableFunctions::PathGetDirectory(application_directory);

    return MakeFullPath(application_directory, L"..\\..\\..\\cspro\\");
}


std::vector<std::wstring> MessageLoader::GetMessageFilenames(bool include_designer_messages)
{
    std::vector<std::wstring> message_filenames;

    auto add_messages = [&](wstring_view file_spec_sv)
    {
        VectorHelpers::Append(message_filenames, DirectoryLister().SetNameFilter(file_spec_sv)
                                                                  .GetPaths(GetCSProDevelopmentDirectory()));
    };

    if( include_designer_messages )
        add_messages(L"CSProDesigner*.mgf");

    add_messages(L"CSProRuntime*.mgf");

    return message_filenames;
}


void MessageLoader::LoadMessageFiles(MessageFile& message_file, bool include_designer_messages, bool* loading_english_messages)
{
    // load all of the message files
    const std::vector<std::wstring> message_filenames = GetMessageFilenames(include_designer_messages);

    for( int pass = 0; pass < 2; ++pass )
    {
        for( const std::wstring& message_filename : message_filenames )
        {
            // first load the English messages
            const bool english_messages = ( message_filename.find(L".en.") != std::wstring::npos );

            if( ( pass == 0 ) == english_messages )
            {
                if( loading_english_messages != nullptr )
                    *loading_english_messages = english_messages;

                TextSourceExternal system_message_text_source(message_filename);
                message_file.Load(system_message_text_source, LogicSettings::Version::V8_0);
            }
        }
    }
}
