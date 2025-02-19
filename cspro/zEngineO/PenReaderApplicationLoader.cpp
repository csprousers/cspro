#include "stdafx.h"
#include "PenReaderApplicationLoader.h"
#include <zMessageO/MessageManager.h>
#include <zMessageO/SystemMessages.h>


PenReaderApplicationLoader::PenReaderApplicationLoader(Application* const application, std::string pen_file_path)
    :   m_application(application)
{
    ASSERT(m_application != nullptr);

    if( pen_file_path.empty() ) // APP_LOAD_TODO eventually the serializer should never be open before this
    {
        m_serializer_APP_LOAD_TODO = &APP_LOAD_TODO_GetArchive();
        return;
    }

    m_serializer = std::make_unique<Serializer>();
    m_serializer->OpenInputArchive(std::move(pen_file_path));
    m_serializer_APP_LOAD_TODO = m_serializer.get();

    if( m_serializer->GetArchiveVersion() < Serializer::GetEarliestSupportedVersion() )
    {
        throw ApplicationLoadException("Compiled applications created using CSPro %0.1f cannot be be run using %s. "
                                       "Please compile your application using the latest version of CSPro.",
                                       static_cast<double>(m_serializer->GetArchiveVersion()) / 100000,
                                       Versioning::CSProVersionText);
    }
}


PenReaderApplicationLoader::~PenReaderApplicationLoader()
{
    if( m_serializer != nullptr )
        m_serializer->CloseArchive();
}


Application* PenReaderApplicationLoader::GetApplication()
{
    *m_serializer >> *m_application;

    return m_application;
}


std::shared_ptr<CDataDict> PenReaderApplicationLoader::GetDictionary(const std::string& /*dictionary_file_path*/)
{
    auto dictionary = std::make_unique<CDataDict>();

    *m_serializer >> *dictionary;

    return dictionary;
}


std::shared_ptr<CDEFormFile> PenReaderApplicationLoader::GetFormFile(const std::string& /*form_file_path*/)
{
    auto form_file = std::make_unique<CDEFormFile>();

    *m_serializer >> *form_file;

    return form_file;
}


std::shared_ptr<CTabSet> PenReaderApplicationLoader::GetTableSpec(const std::string& /*table_spec_file_path*/)
{
    throw ProgrammingErrorException(); // APP_LOAD_TODO
}


std::shared_ptr<MessageManager> PenReaderApplicationLoader::GetSystemMessages()
{
    // when reading from a .pen file, we only need to read in the serialized system messages
    // if they differed from the messages distributed with the installation
    const bool messages_differ = m_serializer_APP_LOAD_TODO->Read<bool>();

    if( messages_differ )
    {
        auto system_message_manager = std::make_unique<MessageManager>();

        *m_serializer_APP_LOAD_TODO >> *system_message_manager;

        SystemMessages::SetMessageFile(system_message_manager->GetSharedMessageFile());

        return system_message_manager;
    }

    else
    {
        return std::make_unique<MessageManager>(SystemMessages::GetSharedMessageFile());
    }
}


std::shared_ptr<MessageManager> PenReaderApplicationLoader::GetUserMessages()
{
    // the user messages are serialized-post compilation so they cannot be read here
    return std::make_unique<MessageManager>();
}


void PenReaderApplicationLoader::ProcessUserMessagesPostCompile(MessageManager& user_message_manager)
{
    *m_serializer_APP_LOAD_TODO >> user_message_manager;
}


void PenReaderApplicationLoader::ProcessResources()
{
    std::vector<std::string> directories;
    m_serializer_APP_LOAD_TODO->SerializePaths(directories, true);

    // when applicable, make sure that files aren't written out above the csentry folder
    const std::optional<std::string> csentry_path =
#ifdef WIN_DESKTOP
        std::nullopt;
#else
        PlatformInterface::GetInstance()->GetCSEntryDirectory();
#endif

    auto valid_directory_check = [&](const std::string& path)
    {
        // make sure that files aren't written to a folder that the user doesn't have access to
        if( csentry_path.has_value() && !SO::StartsWithNoCase(path, *csentry_path) )
            throw ApplicationLoadException("Resources cannot exist above the 'csentry' directory.");

        const std::string directory = PortableFunctions::PathGetDirectory(path);

        if( !PortableFunctions::PathMakeDirectories(directory) )
            throw ApplicationLoadException("A resource directory could not be created: " + directory);
    };

    // check and create the directories
    for( const std::string& directory : directories )
        valid_directory_check(directory);

    // read in the file information and store file paths only for files that need to be updated
    struct FileData { std::optional<std::string> file_path; size_t file_size; };
    std::vector<FileData> file_data;
    size_t files_that_need_updating = 0;

    const size_t number_files = m_serializer_APP_LOAD_TODO->Read<size_t>();

    for( size_t i = 0; i < number_files; ++i )
    {
        std::string file_path;
        m_serializer_APP_LOAD_TODO->SerializePath(file_path, true);

        valid_directory_check(file_path);

        // read the file size and create the file data object
        const size_t file_size = m_serializer_APP_LOAD_TODO->Read<size_t>();

        file_data.emplace_back(FileData { file_path, file_size });

        // don't write the file if it exists and is newer than the date the .pen file was created
        if( PortableFunctions::FileExists(file_path) && PortableFunctions::FileModifiedTime(file_path) > m_serializer_APP_LOAD_TODO->GetArchiveModifiedDate() )
        {
            file_data.back().file_path.reset();
        }

        else
        {
            ++files_that_need_updating;
        }
    }

    try
    {
        for( const FileData& this_file_data : file_data )
        {
            if( files_that_need_updating == 0 )
                return;

            // read the data from the .pen file
            std::vector<std::byte> file_content(this_file_data.file_size);
            m_serializer_APP_LOAD_TODO->Read(file_content.data(), static_cast<int>(file_content.size()));

            // and write it out only when necessary
            if( this_file_data.file_path.has_value() )
            {
                FileIO::Write(*this_file_data.file_path, file_content);
                --files_that_need_updating;
            }
        }
    }

    catch( const FileIO::Exception& exception )
    {
        throw ApplicationLoadException(exception.what());
    }
}
