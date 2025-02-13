#include "stdafx.h"
#include "LanguageChangeEvent.h"

namespace Paradata
{
    void LanguageChangeEvent::SetupTables(Log& log)
    {
        log.CreateTable(ParadataTable::LanguageInfo)
                .AddColumn(_T("question_language_name"), Table::ColumnType::Long)
                .AddColumn(_T("dictionary_language_name"), Table::ColumnType::Long)
                .AddColumn(_T("system_message_language_name"), Table::ColumnType::Long)
                .AddColumn(_T("application_message_language_name"), Table::ColumnType::Long)
            ;

        log.CreateTable(ParadataTable::LanguageChangeEvent)
                .AddColumn(_T("source"), Table::ColumnType::Integer)
                        .AddCode((int)Source::SystemLocale, _T("system_locale"))
                        .AddCode((int)Source::Pff, _T("pff"))
                        .AddCode((int)Source::Logic, _T("logic"))
                        .AddCode((int)Source::Interface, _T("interface"))
                .AddColumn(_T("specified_language_name"), Table::ColumnType::Text)
                .AddColumn(_T("language_info"), Table::ColumnType::Long)
            ;
    }

    LanguageChangeEvent::LanguageChangeEvent(Source source, const CString& specified_language_name,
        std::shared_ptr<NamedObject> questions_language, std::shared_ptr<NamedObject> dictionary_language,
        std::shared_ptr<NamedObject> system_messages_language, std::shared_ptr<NamedObject> application_messages_language)
        :   m_source(source),
            m_specifiedLanguageName(specified_language_name),
            m_questionsLanguage(questions_language),
            m_dictionaryLanguage(dictionary_language),
            m_systemMessagesLanguage(system_messages_language),
            m_applicationMessagesLanguage(application_messages_language)
    {
    }

    void LanguageChangeEvent::Save(Log& log, long base_event_id) const
    {
        // fill the language info table
        Table& language_info_table = log.GetTable(ParadataTable::LanguageInfo);
        long language_info_id = 0;
        language_info_table.Insert(&language_info_id,
            log.AddNamedObject(m_questionsLanguage),
            log.AddNamedObject(m_dictionaryLanguage),
            log.AddNamedObject(m_systemMessagesLanguage),
            log.AddNamedObject(m_applicationMessagesLanguage)
        );

        // fill the language change event table
        Table& language_change_event_table = log.GetTable(ParadataTable::LanguageChangeEvent);
        language_change_event_table.Insert(&base_event_id,
            (int)m_source,
            (LPCTSTR)m_specifiedLanguageName,
            language_info_id
        );
    }
}
