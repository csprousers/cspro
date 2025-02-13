#include "stdafx.h"
#include "FileApplicationLoader.h"
#include <zMessageO/MessageManager.h>
#include <zMessageO/SystemMessages.h>


FileApplicationLoader::FileApplicationLoader(Application* application, std::optional<CString> application_filename/* = std::nullopt*/)
    :   m_application(application),
        m_applicationFilenameToBeLoaded(std::move(application_filename))
{
    ASSERT(m_application != nullptr);
}


Application* FileApplicationLoader::GetApplication()
{
    // silently load the application file if it has not already been loaded
    if( m_applicationFilenameToBeLoaded.has_value() )
    {
        m_application->SetApplicationFilename(*m_applicationFilenameToBeLoaded);

        if( !PortableFunctions::FileIsRegular(*m_applicationFilenameToBeLoaded) )
            throw ApplicationFileNotFoundException(*m_applicationFilenameToBeLoaded, _T("application"));

        try
        {
            m_application->Open(*m_applicationFilenameToBeLoaded, true);
        }

        catch( const CSProException& exception )
        {
            throw ApplicationLoadException(exception.GetErrorMessage());
        }
    }

    return m_application;
}


std::shared_ptr<CDataDict> FileApplicationLoader::GetDictionary(NullTerminatedString dictionary_filename)
{
    if( !PortableFunctions::FileIsRegular(dictionary_filename) )
        throw ApplicationFileNotFoundException(dictionary_filename, _T("dictionary"));

    try
    {
        return CDataDict::InstantiateAndOpen(dictionary_filename, true);
    }

    catch( const CSProException& exception )
    {
        throw ApplicationLoadException(exception.GetErrorMessage());
    }
}


std::shared_ptr<CDEFormFile> FileApplicationLoader::GetFormFile(const CString& form_filename)
{
    auto form_file = std::make_shared<CDEFormFile>();

    if( !PortableFunctions::FileIsRegular(form_filename) )
        throw ApplicationFileNotFoundException(form_filename, _T("form"));

    form_file->SetFileName(form_filename);

    if( !form_file->Open(form_filename, true) )
        throw ApplicationFileLoadException(form_filename, _T("form"));

    return form_file;
}


std::shared_ptr<MessageManager> FileApplicationLoader::GetSystemMessages()
{
    // load the system messages, including any runtime messages in the application directory and
    // any specified as part of the application's message include files
    SystemMessages::LoadMessages(CS2WS(m_application->GetApplicationFilename()), m_application->GetMessageTextSources(), true);

    return std::make_shared<MessageManager>(SystemMessages::GetSharedMessageFile());
}


std::shared_ptr<MessageManager> FileApplicationLoader::GetUserMessages()
{
    auto user_message_manager = std::make_shared<MessageManager>();

    for( const TextSource& message_text_source : VI_V(m_application->GetMessageTextSources()) )
    {
        if( !SystemMessages::IsMessageFilenameSystemMessages(message_text_source.GetFilename()) )
            user_message_manager->Load(message_text_source, m_application->GetLogicSettings().GetVersion());
    }

    return user_message_manager;
}
