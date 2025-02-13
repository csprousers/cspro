#include "stdafx.h"
#include "PenWriterApplicationLoader.h"
#include <zToolsO/DirectoryLister.h>
#include <zToolsO/Tools.h>
#include <zUtilO/ArrUtil.h>
#include <zMessageO/MessageManager.h>
#include <zMessageO/SystemMessages.h>


PenWriterApplicationLoader::PenWriterApplicationLoader(Application* application, std::wstring pen_filename,
                                                       std::optional<CString> application_filename/* = std::nullopt*/)
    :   FileApplicationLoader(application, std::move(application_filename))
{
    ASSERT(m_application != nullptr);

    if( pen_filename.empty() ) // APP_LOAD_TODO eventually the serializer should never be open before this
    {
        m_serializer_APP_LOAD_TODO = &APP_LOAD_TODO_GetArchive();
        return;
    }

    m_serializer = std::make_unique<Serializer>();
    m_serializer->CreateOutputArchive(std::move(pen_filename));
    m_serializer_APP_LOAD_TODO = m_serializer.get();
}


PenWriterApplicationLoader::~PenWriterApplicationLoader()
{
    if( m_serializer != nullptr )
        m_serializer->CloseArchive();
}


Application* PenWriterApplicationLoader::GetApplication()
{
    FileApplicationLoader::GetApplication();

    *m_serializer << *m_application;

    return m_application;
}


std::shared_ptr<CDataDict> PenWriterApplicationLoader::GetDictionary(NullTerminatedString dictionary_filename)
{
    std::shared_ptr<CDataDict> dictionary = FileApplicationLoader::GetDictionary(dictionary_filename);

    *m_serializer << *dictionary;

    return dictionary;
}


std::shared_ptr<CDEFormFile> PenWriterApplicationLoader::GetFormFile(const CString& form_filename)
{
    std::shared_ptr<CDEFormFile> form_file = FileApplicationLoader::GetFormFile(form_filename);

    *m_serializer << *form_file;

    return form_file;
}


std::shared_ptr<MessageManager> PenWriterApplicationLoader::GetSystemMessages()
{
    std::shared_ptr<MessageManager> system_message_manager = FileApplicationLoader::GetSystemMessages();

    // only serialize system messages if they differ from the messages distributed with the installation
    bool messages_differ = SystemMessages::ApplicationUsesCustomMessages();

#ifdef _DEBUG
    // when creating assets, the Installer Generator creates .pen files from the debug directory,
    // but the system messages shouldn't be serialized in these cases
    if( messages_differ && CString(GetCommandLine()).Find(_T("/noSystemMessageSerialization")) >= 0 )
        messages_differ = false;
#endif

    *m_serializer_APP_LOAD_TODO << messages_differ;

    if( messages_differ )
        *m_serializer_APP_LOAD_TODO << *system_message_manager;

    return system_message_manager;
}


std::shared_ptr<MessageManager> PenWriterApplicationLoader::GetUserMessages()
{
    // the messages will be read here but serialized post-compilation
    return FileApplicationLoader::GetUserMessages();
}


void PenWriterApplicationLoader::ProcessUserMessagesPostCompile(MessageManager& user_message_manager)
{
    FileApplicationLoader::ProcessUserMessagesPostCompile(user_message_manager);

    *m_serializer_APP_LOAD_TODO << user_message_manager;
}


void PenWriterApplicationLoader::ProcessResources()
{
    FileApplicationLoader::ProcessResources();

    std::vector<CString> directories;
    std::vector<std::wstring> files;

    const std::wstring& pen_filename = m_serializer_APP_LOAD_TODO->GetArchiveFilename();
    std::wstring pen_directory = PortableFunctions::PathRemoveTrailingSlash(PortableFunctions::PathGetDirectory(pen_filename));

    // get a list of all of the directories and files
    for( const CString& resource_folder : m_application->GetResourceFolders() )
    {
        CString directory = PortableFunctions::PathRemoveTrailingSlash<CString>(resource_folder);

        // don't include the directory where the .pen file is being created
        if( !SO::EqualsNoCase(directory, pen_directory) )
            directories.emplace_back(resource_folder);

        DirectoryLister().SetRecursive()
                         .SetIncludeDirectories()
                         .AddPaths(files, resource_folder);
    }
    
    // process the files read in each directory
    for( auto file_itr = files.begin(); file_itr != files.end(); )
    {
        if( PortableFunctions::FileIsDirectory(*file_itr) )
        {
            directories.emplace_back(PortableFunctions::PathRemoveTrailingSlash<CString>(*file_itr));
            file_itr = files.erase(file_itr);
        }

        // don't allow for the adding of the currently-being-created .pen file
        else if( SO::EqualsNoCase(*file_itr, pen_filename) )
        {
            file_itr = files.erase(file_itr);
        }

        else
        {
            ++file_itr;
        }
    }

    RemoveDuplicateStringsInVectorNoCase(directories);
    RemoveDuplicateStringsInVectorNoCase(files);

    // write out the directories
    m_serializer_APP_LOAD_TODO->SerializeFilenameArray(directories);

    // write out the file names and sizes
    m_serializer_APP_LOAD_TODO->Write(files.size());

    for( const std::wstring& filename : files )
    {
        m_serializer_APP_LOAD_TODO->WriteFilename(filename);
        m_serializer_APP_LOAD_TODO->Write(static_cast<size_t>(PortableFunctions::FileSize(filename)));
    }

    try
    {
        // write out the file contents
        for( const std::wstring& filename : files )
        {
            std::unique_ptr<const std::vector<std::byte>> file_content = FileIO::Read(filename);

            if( !file_content->empty() )
                m_serializer_APP_LOAD_TODO->Write(file_content->data(), static_cast<int>(file_content->size()));
        }
    }

    catch( const FileIO::Exception& exception )
    {
        throw ApplicationLoadException(exception.GetErrorMessage());
    }
}
