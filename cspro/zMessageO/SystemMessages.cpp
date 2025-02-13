#include "stdafx.h"
#include "SystemMessages.h"
#include "MessageFile.h"
#include <zPlatformO/PlatformInterface.h>
#include <zToolsO/DirectoryLister.h>
#include <zUtilO/ArrUtil.h>
#include <zUtilO/CSProExecutables.h>
#include <zUtilO/TextSourceExternal.h>


namespace
{
    std::shared_ptr<MessageFile> SystemMessageFile;
    bool SystemMessageFileContainsCustomMessages = false;

    constexpr const TCHAR* DESIGNER_SYSTEM_MESSAGES_FILENAME =        _T("CSProDesigner.mgf");
    #define RUNTIME_SYSTEM_MESSAGES_FILENAME_BASE                     _T("CSProRuntime")
    constexpr const TCHAR* RUNTIME_SYSTEM_MESSAGES_FILENAME =         RUNTIME_SYSTEM_MESSAGES_FILENAME_BASE _T(".en.mgf");
    constexpr const TCHAR* RUNTIME_SYSTEM_MESSAGES_FILENAME_PATTERN = RUNTIME_SYSTEM_MESSAGES_FILENAME_BASE _T("*.mgf");


#ifdef WIN_DESKTOP

    std::vector<std::shared_ptr<const TextSource>> GetDefaultSystemMessageFilenames(bool load_designer_messages)
    {
        std::vector<std::shared_ptr<const TextSource>> system_message_text_sources;

        auto add_message_text_source = [&](const std::wstring& filename)
        {
            if( PortableFunctions::FileIsRegular(filename) && !IsFilenameInUse(system_message_text_sources, filename) )
                system_message_text_sources.emplace_back(std::make_unique<TextSourceExternal>(filename));
        };

        const std::wstring messages_directory = CSProExecutables::GetApplicationOrAssetsDirectory();

        if( load_designer_messages )
            add_message_text_source(messages_directory + DESIGNER_SYSTEM_MESSAGES_FILENAME);

        // add the default English runtime messages
        add_message_text_source(messages_directory + RUNTIME_SYSTEM_MESSAGES_FILENAME);

        // now check for any other runtime messages in the installation folder
        for( const std::wstring& runtime_message_filename : DirectoryLister().SetNameFilter(RUNTIME_SYSTEM_MESSAGES_FILENAME_PATTERN)
                                                                             .GetPaths(messages_directory) )
        {
            add_message_text_source(runtime_message_filename);
        }

        return system_message_text_sources;
    }


    void LoadSystemMessages(const std::vector<std::shared_ptr<const TextSource>>& system_message_text_sources)
    {
        SystemMessageFile = std::make_shared<MessageFile>();

        for( const TextSource& message_text_source : VI_V(system_message_text_sources) )
            SystemMessageFile->Load(message_text_source, LogicSettings::Version::V8_0);

        ASSERT(SystemMessageFile->GetLoadParserMessages().empty());
    }

#endif // WIN_DESKTOP

    void LoadSystemMessagesFromAssets()
    {
        SystemMessageFile = std::make_shared<MessageFile>();

        try
        {
            Serializer message_serializer;
            message_serializer.OpenInputArchive(PortableFunctions::PathAppendToPath(CSProExecutables::GetApplicationOrAssetsDirectory(), _T("system.mgf")));
            message_serializer & *SystemMessageFile;
            message_serializer.CloseArchive();
        }
        catch( const ApplicationLoadException& ) { }
    }
}


bool SystemMessages::IsMessageFilenameSystemMessages(const std::wstring& message_filename)
{
    return SO::StartsWithNoCase(PortableFunctions::PathGetFilename(message_filename), RUNTIME_SYSTEM_MESSAGES_FILENAME_BASE);
}


#ifdef WIN_DESKTOP

void SystemMessages::LoadMessages(const std::wstring& application_filename, const std::vector<std::shared_ptr<TextSource>>& application_message_text_sources,
                                  bool load_designer_messages)
{
    std::vector<std::shared_ptr<const TextSource>> system_message_text_sources = GetDefaultSystemMessageFilenames(load_designer_messages);
    const size_t default_system_message_text_sources_size = system_message_text_sources.size();

    // only add additional message files that are system message files
    for( const std::shared_ptr<TextSource>& message_text_source : application_message_text_sources )
    {
        if( SystemMessages::IsMessageFilenameSystemMessages(message_text_source->GetFilename()) )
        {
            if( !IsFilenameInUse(system_message_text_sources, message_text_source->GetFilename()) )
                system_message_text_sources.emplace_back(message_text_source);
        }
    }

    // add any runtime message files in the application directory
    const std::wstring application_directory = PortableFunctions::PathGetDirectory(application_filename);

    if( !application_directory.empty() )
    {
        for( const std::wstring& runtime_message_filename : DirectoryLister().SetNameFilter(RUNTIME_SYSTEM_MESSAGES_FILENAME_PATTERN)
                                                                             .GetPaths(application_directory) )
        {
            if( !IsFilenameInUse(system_message_text_sources, runtime_message_filename) )
                system_message_text_sources.emplace_back(std::make_shared<TextSourceExternal>(runtime_message_filename));
        }
    }

    // we only need to reload the messages if the application does not have any additional system message files
    const bool custom_message_files_exist = ( system_message_text_sources.size() != default_system_message_text_sources_size );

    if( SystemMessageFile == nullptr || custom_message_files_exist )
    {
        if( custom_message_files_exist )
            SystemMessageFileContainsCustomMessages = true;

        LoadSystemMessages(system_message_text_sources);
    }
}

#endif // WIN_DESKTOP


MessageFile& SystemMessages::GetMessageFile()
{
    return ( SystemMessageFile != nullptr ) ? *SystemMessageFile :
                                              *GetSharedMessageFile();
}


std::shared_ptr<MessageFile> SystemMessages::GetSharedMessageFile()
{
    if( SystemMessageFile == nullptr )
    {
        // when loading the default set of messages, ignore any errors reading the messages
        try
        {
#ifdef WIN_DESKTOP
            LoadSystemMessages(GetDefaultSystemMessageFilenames(true));
#else
            LoadSystemMessagesFromAssets();
#endif
        }

        catch( const ApplicationLoadException& )
        {
            ASSERT(SystemMessageFile != nullptr);
        }
    }

    return SystemMessageFile;
}


void SystemMessages::SetMessageFile(std::shared_ptr<MessageFile> message_file)
{
    ASSERT(message_file != nullptr);
    SystemMessageFile = std::move(message_file);
}


bool SystemMessages::ApplicationUsesCustomMessages()
{
#ifdef WIN_DESKTOP
    // check if the default files differ from what should have shipped with the installation
    if( !SystemMessageFileContainsCustomMessages )
    {
        const time_t application_modified_time = PortableFunctions::FileModifiedTime(CSProExecutables::GetModuleFilename());

        const std::vector<std::shared_ptr<const TextSource>>& system_message_text_sources = GetDefaultSystemMessageFilenames(false);

        for( const TextSource& message_text_source : VI_V(system_message_text_sources) )
        {
            if( PortableFunctions::FileModifiedTime(message_text_source.GetFilename()) != application_modified_time )
            {
                SystemMessageFileContainsCustomMessages = true;
                break;
            }
        }
    }
#endif

    return SystemMessageFileContainsCustomMessages;
}
