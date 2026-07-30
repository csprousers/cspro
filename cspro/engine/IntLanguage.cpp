#include "StandardSystemIncludes.h"
#include "Interpreter.h"
#include "Engine.h"
#include <zEngineO/EngineDictionary.h>
#include <zAppO/Application.h>
#include <zMessageO/MessageFile.h>
#include <zMessageO/MessageManager.h>
#include <zDictO/DDClass.h>
#include <zFormO/FormFile.h>
#include <zBridgeO/NPff.h>
#include <zParadataO/Logger.h>
#include <zCapiO/CapiQuestionManager.h>
#include <CSEntry/UWM.h>


bool CIntDriver::SetLanguage(const std::string_view language_name_sv, SetLanguageSource language_source, bool show_failure_message/* = false*/)
{
    bool language_exists = false;

    const std::string formatted_language_name = SO::ToUpper(SO::Trim(language_name_sv));

    if( !formatted_language_name.empty() )
    {
        // check for the language in the QSF texts
        if( Issamod == ModuleType::Entry )
        {
            CEntryDriver* pEntryDriver = assert_cast<CEntryDriver*>(m_pEngineDriver);

            const auto& languages = pEntryDriver->GetQuestMgr()->GetLanguages();
            const auto& lookup = std::find_if(languages.cbegin(), languages.cend(),
                                              [&](const Language& l) { return ( l.GetName() == formatted_language_name ); });

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
                const std::optional<size_t> language_index = pDataDict->IsLanguageDefined(formatted_language_name);

                if( language_index.has_value() )
                {
                    language_exists = true;
                    pDataDict->SetCurrentLanguage(*language_index);

                    // refresh the associated form file
                    for( const auto& form_file : m_pEngineDriver->GetApplication()->GetRuntimeFormFiles() )
                    {
                        if( form_file->GetDictionary() == pDataDict )
                        {
                            form_file->RefreshAssociatedFieldText();
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

        // inform the UI of language changes to force the redrawing of rosters
        if( Issamod == ModuleType::Entry )
            WindowsDesktopMessage::Send(UWM::CSEntry::ShowCapi, 0, 1);
    }

    else if( show_failure_message )
    {
        issaerror(MessageType::Error, 91118, formatted_language_name.c_str());
    }


    if( Paradata::Logger::IsOpen() )
    {
        std::string capi_language_name;

        // questions
        if( Issamod == ModuleType::Entry )
            capi_language_name = ((CEntryDriver*)m_pEngineDriver)->GetQuestMgr()->GetCurrentLanguage().GetName();

        // dictionary
        const CDataDict* pDict = m_pEngineDriver->UseNewDriver() ? &m_engineData->engine_dictionaries.front()->GetDictionary() :
                                                                   DIP(0)->GetDataDict();
        const std::string& dictionary_language_name = pDict->GetCurrentLanguage().GetName();

        // messages
        const std::string& system_messages_language_name = m_pEngineDriver->GetSystemMessageManager().GetMessageFile().GetCurrentLanguageName();
        const std::string& application_messages_language_name = m_pEngineDriver->GetUserMessageManager().GetMessageFile().GetCurrentLanguageName();

        m_paradataDriver->RegisterAndLogEvent(std::make_unique<Paradata::LanguageChangeEvent>(
            static_cast<Paradata::LanguageChangeEvent::Source>(language_source), std::string(language_name_sv),
            m_paradataDriver->CreateObject(Paradata::NamedObject::Type::Language, capi_language_name),
            m_paradataDriver->CreateObject(Paradata::NamedObject::Type::Language, dictionary_language_name),
            m_paradataDriver->CreateObject(Paradata::NamedObject::Type::Language, system_messages_language_name),
            m_paradataDriver->CreateObject(Paradata::NamedObject::Type::Language, application_messages_language_name)));
    }

    return language_exists;
}


void CIntDriver::SetStartupLanguage()
{
    std::string language_name = UTF8_TODO::GetUtf8(m_pEngineDriver->m_pPifFile->GetStartLanguageString());
    SetLanguageSource language_source = SetLanguageSource::Pff;

    if( language_name.empty() )
    {
        language_name = GetLocaleLanguage();

        // the locale will look like en_US, so get rid of _US if present
        const size_t hyphen_pos = language_name.find('_');

        if( hyphen_pos != std::string::npos )
            language_name.erase(hyphen_pos);

        language_source = SetLanguageSource::SystemLocale;
    }

    SetLanguage(language_name, language_source);
}


std::vector<Language> CIntDriver::GetLanguages(const bool include_only_capi_languages/* = true*/) const
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


double CIntDriver::ex_getlanguage(int  /*program_index*/)
{
    return AssignString(m_pEngineDriver->GetCurrentLanguageName());
}


double CIntDriver::ex_setlanguage(const int program_index)
{
    const auto& fnn_node = GetNode<FNN_NODE>(program_index);
    const SharableString language_name = Evaluate<SharableString>(fnn_node.fn_expr[0]);

    return SetLanguage(*language_name, CIntDriver::SetLanguageSource::Logic);
}


double CIntDriver::ex_tr(const int program_index)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);
    const MessageFile& user_message_file = m_pEngineDriver->GetUserMessageManager().GetMessageFile();

    if( static_cast<DataType>(va_node.arguments[0]) == DataType::String )
    {
        const SharableString text = Evaluate<SharableString>(va_node.arguments[1]);
        return AssignString(user_message_file.GetTranslation(text));
    }

    else
    {
        ASSERT(static_cast<DataType>(va_node.arguments[0]) == DataType::Numeric);

        const int message_number = Evaluate<int>(va_node.arguments[1]);
        return AssignString(user_message_file.GetMessageText(message_number));
    }
}
