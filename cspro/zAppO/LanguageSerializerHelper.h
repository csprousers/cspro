#pragma once

#include <zAppO/Language.h>
#include <zToolsO/SerializerHelper.h>


class LanguageSerializerHelper : public SerializerHelper::Helper
{
public:
    LanguageSerializerHelper(const std::vector<Language>& languages);

    size_t GetNumLanguages() const { return m_languages.size(); }

    const std::string& GetLanguageName(size_t index) const;

    std::optional<size_t> GetLanguageIndex(std::string_view language_name_sv) const;

private:
    const std::vector<Language>& m_languages;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline LanguageSerializerHelper::LanguageSerializerHelper(const std::vector<Language>& languages)
    :   m_languages(languages)
{
    ASSERT(!m_languages.empty());
}


inline const std::string& LanguageSerializerHelper::GetLanguageName(const size_t index) const
{
    ASSERT(index < m_languages.size());
    return m_languages[index].GetName();
}


inline std::optional<size_t> LanguageSerializerHelper::GetLanguageIndex(const std::string_view language_name_sv) const
{
    for( size_t i = 0; i < m_languages.size(); ++i )
    {
        if( SO::EqualsNoCase(language_name_sv, m_languages[i].GetName()) )
            return i;
    }

    return std::nullopt;
}
