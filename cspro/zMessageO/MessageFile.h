#pragma once

#include <zMessageO/zMessageO.h>
#include <zAppO/LogicSettings.h>
#include <zLogicO/ParserMessage.h>

class TextSource;


class ZMESSAGEO_API MessageFile
{
protected:
    struct LanguageSet
    {
        std::string language_name;
        std::map<int, SharableString> numbered_messages;
        std::map<std::string, SharableString> text_translations;

        void serialize(Serializer& ar);
    };

public:
    MessageFile();

    void Load(const TextSource& text_source, LogicSettings::Version version);

    // Returns any errors/warnings logged while loading the messages.
    const std::vector<Logic::ParserMessage>& GetLoadParserMessages() const { return m_loadParserMessages; }

    void InsertMessage(int message_number, SharableString message_text);

    bool ChangeLanguage(std::string_view language_name_sv);

    size_t GetCurrentLanguageSetIndex() const         { return m_currentLanguageSetIndex; }
    const std::string& GetCurrentLanguageName() const { return GetCurrentLanguageSet().language_name; }

    // Returns the message text. If not defined, the SharableString will not be set.
    SharableString GetMessageTextWithNoDefaultMessage(int message_number) const;

    // Returns the message text. If not defined, returns an "invalid message number" message.
    SharableString GetMessageText(int message_number) const;

    // Returns the message text (with no default message) along with the current language used to retrieved the text.
    std::tuple<SharableString, std::string> GetMessageTextAndCurrentLanguage(int message_number) const;

    SharableString GetTranslation(const SharableString& original_text) const;

    void serialize(Serializer& ar);

private:
    class Compiler;

    const LanguageSet& GetCurrentLanguageSet() const { ASSERT(m_currentLanguageSetIndex < m_languageSets.size()); return m_languageSets[m_currentLanguageSetIndex]; }
    LanguageSet& GetCurrentLanguageSet()             { ASSERT(m_currentLanguageSetIndex < m_languageSets.size()); return m_languageSets[m_currentLanguageSetIndex]; }

    size_t GetOrCreateLanguageSet(std::string_view language_name_sv);

    SharableString GetMessageTextWithNoDefaultMessage(int message_number, const LanguageSet** language_set_used) const;

#ifdef _DEBUG
protected:
    // used only by the Messages Processor
    virtual void LoadedMessageNumber(int message_number) { message_number; }
#endif

protected:
    std::vector<LanguageSet> m_languageSets;

private:
    size_t m_currentLanguageSetIndex;
    mutable std::unique_ptr<std::map<std::tuple<size_t, int>, SharableString>> m_invalidMessageNumberMessages;

    // compilation-only variables
    std::vector<Logic::ParserMessage> m_loadParserMessages;
    std::set<int> m_primaryLanguageMessageNumbersDefinedFromOtherLanguage;
};
