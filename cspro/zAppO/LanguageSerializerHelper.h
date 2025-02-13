#pragma once

#include <zAppO/Language.h>
#include <zToolsO/SerializerHelper.h>


class LanguageSerializerHelper : public SerializerHelper::Helper
{
public:
    LanguageSerializerHelper(const std::vector<Language>& languages)
        :   m_languages(languages)
    {
        ASSERT(!m_languages.empty());
    }

    size_t GetNumLanguages() const
    {
        return m_languages.size();
    }

    const std::wstring& GetLanguageName(size_t index) const
    {
        ASSERT(index < m_languages.size());
        return m_languages[index].GetName();
    }

    std::optional<size_t> GetLanguageIndex(const std::wstring& language_name) const
    {
        for( size_t i = 0; i < m_languages.size(); ++i )
        {
            if( SO::EqualsNoCase(language_name, m_languages[i].GetName()) )
                return i;
        }

        return std::nullopt;
    }

private:
    const std::vector<Language>& m_languages;
};
