#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include "Engine.h"
#include <zEngineO/EngineDictionary.h>
#include <zAppO/Application.h>
#include <zMessageO/MessageFile.h>
#include <zMessageO/MessageManager.h>
#include <zDictO/DDClass.h>
#include <zFormO/FormFile.h>
#include <ZBRIDGEO/npff.h>
#include <zParadataO/Logger.h>
#include <zCapiO/CapiQuestionManager.h>
#include <CSEntry/UWM.h>


bool CIntDriver::SetLanguage(wstring_view language_name, SetLanguageSource language_source, bool show_failure_message/* = false*/)
{
    bool language_exists = false;

    std::wstring formatted_language_name = SO::ToUpper(SO::Trim(language_name));

    if( !formatted_language_name.empty() )
    {
        // check for the language in the QSF texts
        if( Issamod == ModuleType::Entry )
        {
            CEntryDriver* pEntryDriver = assert_cast<CEntryDriver*>(m_pEngineDriver);

            const auto& languages = pEntryDriver->GetQuestMgr()->GetLanguages();
            const auto& lookup = std::find_if(languages.cbegin(), languages.cend(),
                                              [&](const Language& l) { return l.GetName() == formatted_language_name; });

            if( lookup != languages.cend() )
            {
                language_exists = true;
                pEntryDriver->GetQuestMgr()->SetCurrentLanguage(formatted_language_name);
            }
        }

        // check for the language in the dictionaries
        for( const DICT* pDicT : m_engineData->dictionaries_pre80 )
        {
            const CDataDict* pDataDict = pDicT->GetDataDict();

            if( pDataDict != nullptr )
            {
                std::optional<size_t> language_index = pDataDict->IsLanguageDefined(formatted_language_name);

                if( language_index.has_value() )
                {
                    language_exists = true;
                    pDataDict->SetCurrentLanguage(*language_index);

                    // refresh the associated form file
                    for( const auto& pFormFile : m_pEngineDriver->GetApplication()->GetRuntimeFormFiles() )
                    {
                        if( pFormFile->GetDictionary() == pDataDict )
                        {
                            pFormFile->RefreshAssociatedFieldText();
                            break;
                        }
                    }
                }
            }
        }

        // change the language of the messages
        language_exists |= m_pEngineDriver->GetSystemMessageManager().GetMessageFile().ChangeLanguage(formatted_language_name);
        language_exists |= m_pEngineDriver->GetUserMessageManager().GetMessageFile().ChangeLanguage(formatted_language_name);
    }

    if( language_exists )
    {
        m_pEngineDriver->SetCurrentLanguageName(formatted_language_name);

        if( Issamod == ModuleType::Entry )
        {
#ifdef WIN_DESKTOP
            // inform the interface for refreshing
            CWnd* pMainWnd = AfxGetApp()->GetMainWnd();
            if( pMainWnd && IsWindow(pMainWnd->GetSafeHwnd()) )
                pMainWnd->SendMessage(UWM::CSEntry::ShowCapi, 0, 1); //inform the UI of change languages to force redraw rosters
#endif
        }
    }

    else if( show_failure_message )
    {
        issaerror(MessageType::Error, 91118, std::wstring(formatted_language_name).c_str());
    }


    if( Paradata::Logger::IsOpen() )
    {
        std::wstring capi_language_name;

        // questions
        if( Issamod == ModuleType::Entry )
            capi_language_name = ((CEntryDriver*)m_pEngineDriver)->GetQuestMgr()->GetCurrentLanguage().GetName();

        // dictionary
        const CDataDict* pDict = m_pEngineDriver->UseNewDriver() ? &m_engineData->engine_dictionaries.front()->GetDictionary() :
                                                                   DIP(0)->GetDataDict();
        const std::wstring& dictionary_language_name = pDict->GetCurrentLanguage().GetName();

        // messages
        const std::wstring& system_messages_language_name = m_pEngineDriver->GetSystemMessageManager().GetMessageFile().GetCurrentLanguageName();
        const std::wstring& application_messages_language_name = m_pEngineDriver->GetUserMessageManager().GetMessageFile().GetCurrentLanguageName();

        m_pParadataDriver->RegisterAndLogEvent(std::make_shared<Paradata::LanguageChangeEvent>(
            (Paradata::LanguageChangeEvent::Source)language_source, language_name,
            m_pParadataDriver->CreateObject(Paradata::NamedObject::Type::Language, capi_language_name),
            m_pParadataDriver->CreateObject(Paradata::NamedObject::Type::Language, dictionary_language_name),
            m_pParadataDriver->CreateObject(Paradata::NamedObject::Type::Language, system_messages_language_name),
            m_pParadataDriver->CreateObject(Paradata::NamedObject::Type::Language, application_messages_language_name)));
    }

    return language_exists;
}


void CIntDriver::SetStartupLanguage()
{
    CString language_name = m_pEngineDriver->m_pPifFile->GetStartLanguageString();
    SetLanguageSource language_source = SetLanguageSource::Pff;

    if( language_name.IsEmpty() )
    {
        language_name = GetLocaleLanguage();

        // the locale will look like en_US, so get rid of _US
        int hyphen_pos = language_name.Find('_');

        if( hyphen_pos > 0 )
            language_name = language_name.Left(hyphen_pos);

        language_source = SetLanguageSource::SystemLocale;
    }

    SetLanguage(language_name, language_source);
}


std::vector<Language> CIntDriver::GetLanguages(bool include_only_capi_languages/* = true*/) const
{
    ASSERT(Issamod == ModuleType::Entry);
    CEntryDriver* pEntryDriver = assert_cast<CEntryDriver*>(m_pEngineDriver);
    std::vector<Language> languages = pEntryDriver->GetQuestMgr()->GetLanguages();

    if( !include_only_capi_languages )
    {
        // add dictionary languages (when there are more than one)
        for( const DICT* pDicT : m_engineData->dictionaries_pre80 )
        {
            const CDataDict* pDataDict = pDicT->GetDataDict();

            if( pDataDict != nullptr && pDataDict->GetLanguages().size() > 1 )
            {
                for( const auto& dictionary_language : pDataDict->GetLanguages() )
                {
                    const auto& language_search = std::find_if(languages.cbegin(), languages.cend(),
                        [&](const auto& language)
                        {
                            return ( language.GetName() == dictionary_language.GetName() );
                        });

                    if( language_search == languages.cend() )
                        languages.emplace_back(dictionary_language);
                }
            }
        }
    }

    return languages;
}


double CIntDriver::exgetlanguage(int /*iExpr*/) // GHM 20100309
{
    return AssignAlphaValue(m_pEngineDriver->GetCurrentLanguageName());
}


double CIntDriver::exsetlanguage(int iExpr) // GHM 20100309
{
    const auto& fnn_node = GetNode<FNN_NODE>(iExpr);
    std::wstring language_name = EvalAlphaExpr(fnn_node.fn_expr[0]);

    return SetLanguage(language_name, CIntDriver::SetLanguageSource::Logic) ? 1 : 0; // false for no error message
}


double CIntDriver::extr(int iExpr)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(iExpr);
    const MessageFile& user_message_file = m_pEngineDriver->GetUserMessageManager().GetMessageFile();

    if( static_cast<DataType>(va_node.arguments[0]) == DataType::String )
    {
        std::wstring text = EvalAlphaExpr(va_node.arguments[1]);
        return AssignAlphaValue(user_message_file.GetTranslation(std::move(text)));
    }

    else
    {
        ASSERT(static_cast<DataType>(va_node.arguments[0]) == DataType::Numeric);

        const int message_number = static_cast<int>(evalexpr(va_node.arguments[1]));
        return AssignAlphaValue(user_message_file.GetMessageText(message_number));
    }
}
