#include "stdafx.h"
#include "PenReaderApplicationLoader.h"
#include <zUtilO/Versioning.h>
#include <zMessageO/MessageManager.h>
#include <zMessageO/SystemMessages.h>


PenReaderApplicationLoader::PenReaderApplicationLoader(Application* application, std::wstring pen_filename)
    :   m_application(application)
{
    ASSERT(m_application != nullptr);

    if( pen_filename.empty() ) // APP_LOAD_TODO eventually the serializer should never be open before this
    {
        m_serializer_APP_LOAD_TODO = &APP_LOAD_TODO_GetArchive();
        return;
    }

    m_serializer = std::make_unique<Serializer>();
    m_serializer->OpenInputArchive(std::move(pen_filename));
    m_serializer_APP_LOAD_TODO = m_serializer.get();
    
    if( m_serializer->GetArchiveVersion() < Serializer::GetEarliestSupportedVersion() )
    {
        throw ApplicationLoadException(_T("Compiled applications created using CSPro %0.1f cannot be be run using %s. ")
                                       _T("Please compile your application using the latest version of CSPro."),
                                       static_cast<double>(m_serializer->GetArchiveVersion()) / 100000,
                                       CSPRO_VERSION);
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


std::shared_ptr<CDataDict> PenReaderApplicationLoader::GetDictionary(NullTerminatedString /*dictionary_filename*/)
{
    auto dictionary = std::make_shared<CDataDict>();

    *m_serializer >> *dictionary;

    return dictionary;
}


std::shared_ptr<CDEFormFile> PenReaderApplicationLoader::GetFormFile(const CString& /*form_filename*/)
{
    auto form_file = std::make_shared<CDEFormFile>();

    *m_serializer >> *form_file;

    return form_file;
}


std::shared_ptr<MessageManager> PenReaderApplicationLoader::GetSystemMessages()
{
    // when reading from a .pen file, we only need to read in the serialized system messages
    // if they differed from the messages distributed with the installation
    bool messages_differ = m_serializer_APP_LOAD_TODO->Read<bool>();

    if( messages_differ )
    {
        auto system_message_manager = std::make_shared<MessageManager>();

        *m_serializer_APP_LOAD_TODO >> *system_message_manager;

        SystemMessages::SetMessageFile(system_message_manager->GetSharedMessageFile());

        return system_message_manager;
    }

    else
    {
        return std::make_shared<MessageManager>(SystemMessages::GetSharedMessageFile());
    }
}


std::shared_ptr<MessageManager> PenReaderApplicationLoader::GetUserMessages()
{
    // the user messages are serialized-post compilation so they cannot be read here
    return std::make_shared<MessageManager>();
}


void PenReaderApplicationLoader::ProcessUserMessagesPostCompile(MessageManager& user_message_manager) 
{
    *m_serializer_APP_LOAD_TODO >> user_message_manager;
}


void PenReaderApplicationLoader::ProcessResources()
{
    std::vector<CString> directories;
    m_serializer_APP_LOAD_TODO->SerializeFilenameArray(directories, true);

    std::optional<CString> csentry_path;

#ifndef WIN_DESKTOP
    // make sure that files aren't written out above the csentry folder
    csentry_path = WS2CS(PlatformInterface::GetInstance()->GetCSEntryDirectory());
    csentry_path->MakeLower();
#endif

    auto valid_directory_check = [&](const CString& path)
    {
        // make sure that files aren't written to a folder that the user doesn't have access to
        if( csentry_path.has_value() )
        {
            CString path_check = path;
            path_check.MakeLower();

            if( path_check.Find(*csentry_path) != 0 )
                throw ApplicationLoadException(_T("Resource folders and files cannot exist above the \"csentry\" directory."));
        }

        std::wstring directory = PortableFunctions::PathGetDirectory(path);

        if( !PortableFunctions::PathMakeDirectories(directory) )
            throw ApplicationLoadException(_T("The resource folder %s could not be created."), directory.c_str());
    };

    // check and create the directories
    for( const CString& directory : directories )
        valid_directory_check(directory);

    // read in the file information and store filenames only for files that need to be updated
    struct FileData { std::optional<CString> filename; size_t file_size; };
    std::vector<FileData> file_data;
    size_t files_that_need_updating = 0;

    size_t number_files = m_serializer_APP_LOAD_TODO->Read<size_t>();

    for( size_t i = 0; i < number_files; ++i )
    {
        CString filename;
        m_serializer_APP_LOAD_TODO->SerializeFilename(filename, true);

        valid_directory_check(filename);

        // read the file size and create the file data object
        size_t file_size = m_serializer_APP_LOAD_TODO->Read<size_t>();

        file_data.emplace_back(FileData { filename, file_size });

        // don't write the file if it exists and is newer than the date the .pen file was created
        if( PortableFunctions::FileExists(filename) && PortableFunctions::FileModifiedTime(filename) > m_serializer_APP_LOAD_TODO->GetArchiveModifiedDate() )
        {
            file_data.back().filename.reset();
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

            if( !file_content.empty() )
                m_serializer_APP_LOAD_TODO->Read(file_content.data(), (int)file_content.size());

            // and write it out only when necessary
            if( this_file_data.filename.has_value() )
            {
                FileIO::Write(*this_file_data.filename, file_content);
                --files_that_need_updating;
            }
        }
    }
        
    catch( const FileIO::Exception& exception )
    {
        throw ApplicationLoadException(exception.GetErrorMessage());
    }
}
