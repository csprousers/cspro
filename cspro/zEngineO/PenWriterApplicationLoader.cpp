#include "stdafx.h"
#include "PenWriterApplicationLoader.h"
#include <zToolsO/DirectoryLister.h>
#include <zToolsO/Tools.h>
#include <zUtilO/ArrUtil.h>
#include <zMessageO/MessageManager.h>
#include <zMessageO/SystemMessages.h>
#include <zDictO/DictionaryIterator.h>


PenWriterApplicationLoader::PenWriterApplicationLoader(Application* const application, std::string pen_file_path,
                                                       std::optional<std::string> application_file_path/* = std::nullopt*/)
    :   FileApplicationLoader(application, std::move(application_file_path))
{
    ASSERT(m_application != nullptr);

    if( pen_file_path.empty() ) // APP_LOAD_TODO eventually the serializer should never be open before this
    {
        m_serializer_APP_LOAD_TODO = &APP_LOAD_TODO_GetArchive();
        return;
    }

    m_serializer = std::make_unique<Serializer>();
    m_serializer->CreateOutputArchive(std::move(pen_file_path));
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


std::shared_ptr<CDataDict> PenWriterApplicationLoader::GetDictionary(const std::string& dictionary_file_path)
{
    std::shared_ptr<CDataDict> dictionary = FileApplicationLoader::GetDictionary(dictionary_file_path);

    *m_serializer << *dictionary;

    ProcessDictionaryValueSetImages(*dictionary);

    return dictionary;
}


std::shared_ptr<CDEFormFile> PenWriterApplicationLoader::GetFormFile(const std::string& form_file_path)
{
    std::shared_ptr<CDEFormFile> form_file = FileApplicationLoader::GetFormFile(form_file_path);

    *m_serializer << *form_file;

    return form_file;
}


std::shared_ptr<CTabSet> PenWriterApplicationLoader::GetTableSpec(const std::string& /*table_spec_file_path*/)
{
    throw ProgrammingErrorException(); // APP_LOAD_TODO
}


std::shared_ptr<MessageManager> PenWriterApplicationLoader::GetSystemMessages()
{
    std::shared_ptr<MessageManager> system_message_manager = FileApplicationLoader::GetSystemMessages();

    // only serialize system messages if they differ from the messages distributed with the installation
    bool messages_differ = SystemMessages::ApplicationUsesCustomMessages();

#ifdef _DEBUG
    // when creating assets, the Installer Generator creates .pen files from the debug directory,
    // but the system messages shouldn't be serialized in these cases
    if( messages_differ && std::wstring(GetCommandLine()).find(L"/noSystemMessageSerialization") != std::wstring::npos )
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


void PenWriterApplicationLoader::AddResource(std::string path)
{
    ASSERT(path == PortableFunctions::PathRemoveTrailingSlash(path));

    if( PortableFunctions::FileIsDirectory(path) )
    {
        m_resourceDirectories.emplace_back(std::move(path));
    }

    else
    {
        m_resourceDirectories.emplace_back(PortableFunctions::PathRemoveTrailingSlash(PortableFunctions::PathGetDirectory(path)));
        m_resourceFilePaths.emplace_back(std::move(path));
    }
}


void PenWriterApplicationLoader::ProcessDictionaryValueSetImages(const CDataDict& dictionary)
{
    // potentially add value set images to resources
    const DictionaryDescription* const dictionary_description = m_application->GetDictionaryDescription(dictionary);

    if( dictionary_description == nullptr || !dictionary_description->GetIncludeValueSetImagesInCompiledApplication() )
        return;

    DictionaryIterator::Foreach<DictValue>(dictionary,
        [&](const DictValue& dict_value)
        {
            if( !dict_value.GetImageFilePath().empty() )
                AddResource(dict_value.GetImageFilePath());
        });
}


void PenWriterApplicationLoader::ProcessResources()
{
    FileApplicationLoader::ProcessResources();

    try
    {
        // get a list of all of the directories and files that should be included in the compiled application
        for( const AppResource& resource : m_application->GetResources() )
        {
            if( !resource.GetIncludeInCompiledApplication() )
                continue;

            if( resource.IsDirectory() )
                m_resourceDirectories.emplace_back(resource.GetPath());

            for( std::string& path: resource.GetEvaluatedPaths(true) )
                AddResource(std::move(path));
        }

        // remove duplicate directories and files
        RemoveDuplicateStringsInVectorNoCase(m_resourceDirectories);
        RemoveDuplicateStringsInVectorNoCase(m_resourceFilePaths);

        // don't include the directory where the .pen file is being created, or the currently-being-created .pen file
        const std::string& pen_file_path = m_serializer_APP_LOAD_TODO->GetArchiveFilePath();
        const std::string pen_directory = PortableFunctions::PathRemoveTrailingSlash(PortableFunctions::PathGetDirectory(pen_file_path));

        RemoveStringInVectorNoCase(m_resourceDirectories, pen_directory);
        RemoveStringInVectorNoCase(m_resourceFilePaths, pen_file_path);

        // write out the directories
        m_serializer_APP_LOAD_TODO->SerializePaths(m_resourceDirectories);

        // write out the file names and sizes
        m_serializer_APP_LOAD_TODO->Write(m_resourceFilePaths.size());

        for( const std::string& file_path : m_resourceFilePaths )
        {
            m_serializer_APP_LOAD_TODO->WritePath(file_path);
            m_serializer_APP_LOAD_TODO->Write(static_cast<size_t>(PortableFunctions::FileSize(file_path)));
        }

        // write out the file contents
        for( const std::string& file_path : m_resourceFilePaths )
        {
            const std::unique_ptr<const std::vector<std::byte>> file_content = FileIO::Read(file_path);

            if( !file_content->empty() )
                m_serializer_APP_LOAD_TODO->Write(file_content->data(), static_cast<int>(file_content->size()));
        }
    }

    catch( const std::exception& exception )
    {
        throw ApplicationLoadException(exception.what());
    }
}
