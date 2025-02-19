#include "stdafx.h"
#include "LanguageChangeEvent.h"

using namespace Paradata;


void LanguageChangeEvent::SetupTables(Log& log)
{
    log.CreateTable(ParadataTable::LanguageInfo)
            .AddColumn("question_language_name", Table::ColumnType::Long)
            .AddColumn("dictionary_language_name", Table::ColumnType::Long)
            .AddColumn("system_message_language_name", Table::ColumnType::Long)
            .AddColumn("application_message_language_name", Table::ColumnType::Long)
        ;

    log.CreateTable(ParadataTable::LanguageChangeEvent)
            .AddColumn("source", Table::ColumnType::Integer)
                    .AddCode(Source::SystemLocale, "system_locale")
                    .AddCode(Source::Pff, "pff")
                    .AddCode(Source::Logic, "logic")
                    .AddCode(Source::Interface, "interface")
            .AddColumn("specified_language_name", Table::ColumnType::Text)
            .AddColumn("language_info", Table::ColumnType::Long)
        ;
}


LanguageChangeEvent::LanguageChangeEvent(Source source, std::string specified_language_name,
                                         std::shared_ptr<NamedObject> questions_language, std::shared_ptr<NamedObject> dictionary_language,
                                         std::shared_ptr<NamedObject> system_messages_language, std::shared_ptr<NamedObject> application_messages_language)
    :   m_source(source),
        m_specifiedLanguageName(std::move(specified_language_name)),
        m_questionsLanguage(std::move(questions_language)),
        m_dictionaryLanguage(std::move(dictionary_language)),
        m_systemMessagesLanguage(std::move(system_messages_language)),
        m_applicationMessagesLanguage(std::move(application_messages_language))
{
}


void LanguageChangeEvent::Save(Log& log, long base_event_id) const
{
    // fill the language info table
    Table& language_info_table = log.GetTable(ParadataTable::LanguageInfo);
    long language_info_id = 0;
    language_info_table.Insert(&language_info_id,
        log.AddNamedObject(m_questionsLanguage.get()),
        log.AddNamedObject(m_dictionaryLanguage.get()),
        log.AddNamedObject(m_systemMessagesLanguage.get()),
        log.AddNamedObject(m_applicationMessagesLanguage.get())
    );

    // fill the language change event table
    Table& language_change_event_table = log.GetTable(ParadataTable::LanguageChangeEvent);
    language_change_event_table.Insert(&base_event_id,
        static_cast<int>(m_source),
        m_specifiedLanguageName.c_str(),
        language_info_id
    );
}
