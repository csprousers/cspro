#include "stdafx.h"
#include "Main.h"
#include <zToolsO/DirectoryLister.h>
#include <zToolsO/Tools.h>
#include <zToolsO/VectorHelpers.h>


std::string MessageLoader::GetCSProDevelopmentDirectory()
{
    auto module_file_name = std::make_unique_for_overwrite<wchar_t[]>(_MAX_PATH);
    GetModuleFileName(nullptr, module_file_name.get(), _MAX_PATH);

    return MakeFullPath(PortableFunctions::PathGetDirectory(TC::ToUtf8(module_file_name.get())), 
                        "..\\..\\..\\cspro\\");
}


std::vector<std::string> MessageLoader::GetMessageFilePaths(const bool include_designer_messages)
{
    std::vector<std::string> message_file_paths;

    auto add_messages = [&](const std::string_view file_spec_sv)
    {
        VectorHelpers::Append(message_file_paths, DirectoryLister().SetNameFilter(file_spec_sv)
                                                                   .GetPaths(GetCSProDevelopmentDirectory()));
    };

    if( include_designer_messages )
        add_messages("CSProDesigner*.mgf");

    add_messages("CSProRuntime*.mgf");

    return message_file_paths;
}


void MessageLoader::LoadMessageFiles(MessageFile& message_file, const bool include_designer_messages, bool* const loading_english_messages)
{
    // load all of the message files
    const std::vector<std::string> message_file_paths = GetMessageFilePaths(include_designer_messages);

    for( int pass = 0; pass < 2; ++pass )
    {
        for( const std::string& message_file_path : message_file_paths )
        {
            // first load the English messages
            const bool english_messages = ( message_file_path.find(".en.") != std::string::npos );

            if( ( pass == 0 ) == english_messages )
            {
                if( loading_english_messages != nullptr )
                    *loading_english_messages = english_messages;

                TextSourceExternal system_message_text_source(message_file_path);
                message_file.Load(system_message_text_source, LogicSettings::Version::V8_0);
            }
        }
    }
}
