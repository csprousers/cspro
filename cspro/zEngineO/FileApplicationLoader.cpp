#include "stdafx.h"
#include "FileApplicationLoader.h"
#include <zMessageO/MessageManager.h>
#include <zMessageO/SystemMessages.h>


FileApplicationLoader::FileApplicationLoader(Application* const application, std::optional<std::string> application_file_path/* = std::nullopt*/)
    :   m_application(application),
        m_applicationFilePathToBeLoaded(std::move(application_file_path))
{
    ASSERT(m_application != nullptr);
}


Application* FileApplicationLoader::GetApplication()
{
    // silently load the application file if it has not already been loaded
    if( m_applicationFilePathToBeLoaded.has_value() )
    {
        m_application->SetApplicationFilePath(*m_applicationFilePathToBeLoaded);

        if( !PortableFunctions::FileIsRegular(*m_applicationFilePathToBeLoaded) )
            throw ApplicationFileNotFoundException(*m_applicationFilePathToBeLoaded, "application");

        try
        {
            m_application->Open(*m_applicationFilePathToBeLoaded, true);
        }

        catch( const CSProException& exception )
        {
            throw ApplicationLoadException(exception.what());
        }
    }

    return m_application;
}


std::shared_ptr<CDataDict> FileApplicationLoader::GetDictionary(const std::string& dictionary_file_path)
{
    if( !PortableFunctions::FileIsRegular(dictionary_file_path) )
        throw ApplicationFileNotFoundException(dictionary_file_path, "dictionary");

    try
    {
        return CDataDict::InstantiateAndOpen(dictionary_file_path, true);
    }

    catch( const CSProException& exception )
    {
        throw ApplicationLoadException(exception.what());
    }
}


std::shared_ptr<CDEFormFile> FileApplicationLoader::GetFormFile(const std::string& form_file_path)
{
    auto form_file = std::make_unique<CDEFormFile>();

    if( !PortableFunctions::FileIsRegular(form_file_path) )
        throw ApplicationFileNotFoundException(form_file_path, "form");

    form_file->SetFilePath(form_file_path);

    if( !form_file->Open(form_file_path, true) )
        throw ApplicationFileLoadException(form_file_path, "form");

    return form_file;
}


std::shared_ptr<CTabSet> FileApplicationLoader::GetTableSpec(const std::string& /*table_spec_file_path*/)
{
    throw ProgrammingErrorException(); // APP_LOAD_TODO
}


std::shared_ptr<MessageManager> FileApplicationLoader::GetSystemMessages()
{
    // load the system messages, including any runtime messages in the application directory and
    // any specified as part of the application's message include files
    std::vector<std::shared_ptr<const TextSource>> additional_message_text_sources;

    for( const AppMessageFile& app_message_file : m_application->GetMessageFiles() )
    {
        if( app_message_file.GetType() == AppMessageFile::Type::System )
            additional_message_text_sources.emplace_back(app_message_file.GetSharedTextSource());
    }

    SystemMessages::LoadMessages(m_application->GetApplicationFilePath(), additional_message_text_sources);

    return std::make_unique<MessageManager>(SystemMessages::GetSharedMessageFile());
}


std::shared_ptr<MessageManager> FileApplicationLoader::GetUserMessages()
{
    auto user_message_manager = std::make_unique<MessageManager>();

    for( const AppMessageFile& app_message_file : m_application->GetMessageFiles() )
    {
        if( app_message_file.GetType() == AppMessageFile::Type::User )
            user_message_manager->Load(app_message_file.GetTextSource(), m_application->GetLogicSettings().GetVersion());
    }

    return user_message_manager;
}
