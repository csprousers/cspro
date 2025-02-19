#pragma once

#include <zParadataO/Event.h>

namespace Paradata { class LanguageChangeEvent; }


class ZPARADATAO_API Paradata::LanguageChangeEvent : public Event
{
    DECLARE_PARADATA_EVENT(LanguageChangeEvent)

public:
    enum class Source
    {
        SystemLocale,
        Pff,
        Logic,
        Interface
    };

public:
    LanguageChangeEvent(Source source, std::string specified_language_name,
                        std::shared_ptr<NamedObject> questions_language, std::shared_ptr<NamedObject> dictionary_language,
                        std::shared_ptr<NamedObject> system_messages_language, std::shared_ptr<NamedObject> application_messages_language);

private:
    Source m_source;
    std::string m_specifiedLanguageName;
    std::shared_ptr<NamedObject> m_questionsLanguage;
    std::shared_ptr<NamedObject> m_dictionaryLanguage;
    std::shared_ptr<NamedObject> m_systemMessagesLanguage;
    std::shared_ptr<NamedObject> m_applicationMessagesLanguage;
};
