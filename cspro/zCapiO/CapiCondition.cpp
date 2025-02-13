#include "StdAfx.h"
#include "CapiCondition.h"


CapiCondition::CapiCondition()
    :   CapiCondition(CString())
{
    static_assert(Serializer::GetEarliestSupportedVersion() < Serializer::Iteration_7_6_000_1, "when removing pre-7.6 runtime support, remove m_minOcc and m_maxOcc");
}

CapiCondition::CapiCondition(const CString& logic)
    :   CapiCondition(logic, -1, -1)
{
}

CapiCondition::CapiCondition(const CString& logic, int min_occ, int max_occ)
    :   m_logic(logic),
        m_minOcc(min_occ),
        m_maxOcc(max_occ)
{
}


void CapiCondition::SetMinMaxOcc(int min, int max)
{
    m_minOcc = min;
    m_maxOcc = max;
}


CapiText CapiCondition::GetText(const std::wstring& language_name, CapiTextType type) const
{
    if (type == CapiTextType::QuestionText)
        return GetQuestionText(language_name);
    else
        return GetHelpText(language_name);
}


CapiText CapiCondition::GetQuestionText(const std::wstring& language_name) const
{
    auto it = m_questionTexts.find(language_name);
    return it == m_questionTexts.end() ? CapiText() : it->second;
}


CapiText CapiCondition::GetHelpText(const std::wstring& language_name) const
{
    auto it = m_helpTexts.find(language_name);
    return it == m_helpTexts.end() ? CapiText() : it->second;
}


void CapiCondition::SetText(const CString& text, const std::wstring& language_name, CapiTextType type)
{
    if (type == CapiTextType::QuestionText)
        SetQuestionText(text, language_name);
    else
        SetHelpText(text, language_name);
}


void CapiCondition::SetQuestionText(const CString& text, const std::wstring& language_name)
{
    m_questionTexts[language_name] = text;
}


void CapiCondition::SetHelpText(const CString& text, const std::wstring& language_name)
{
    m_helpTexts[language_name] = text;
}


void CapiCondition::DeleteLanguage(const std::wstring& language_name)
{
    m_questionTexts.erase(language_name);
    m_helpTexts.erase(language_name);
}


void CapiCondition::ModifyLanguage(const std::wstring& old_language_name, const std::wstring& new_language_name)
{
    auto modify = [&](std::map<std::wstring, CapiText>& texts)
    {
        auto it = texts.find(old_language_name);

        if( it != texts.end() )
        {
            CapiText capi_text = it->second;
            texts.erase(it);
            texts[new_language_name] = capi_text;
        }
    };

    modify(m_questionTexts);
    modify(m_helpTexts);
}


CREATE_ENUM_JSON_SERIALIZER(CapiTextType,
    { CapiTextType::QuestionText, _T("question") },
    { CapiTextType::HelpText,     _T("help") })

void CapiCondition::WriteJson(JsonWriter& json_writer) const
{
    json_writer.BeginObject();

    json_writer.WriteIfNotBlank(JK::logic, m_logic);

    if( json_writer.Verbose() || !m_questionTexts.empty() || !m_helpTexts.empty() )
    {
        json_writer.BeginArray(JK::texts);

        auto write_texts = [&](CapiTextType type, const std::map<std::wstring, CapiText>& texts)
        {
            for( const auto& [language, capi_text] : texts )
            {
                if( SO::IsWhitespace(capi_text.GetText()) )
                    continue;

                json_writer.BeginObject()
                           .Write(JK::language, language)
                           .Write(JK::type, type)
                           .Write(JK::html, capi_text)
                           .EndObject();
            }
        };

        write_texts(CapiTextType::QuestionText, m_questionTexts);
        write_texts(CapiTextType::HelpText, m_helpTexts);

        json_writer.EndArray();
    }

    json_writer.EndObject();
}


void CapiCondition::serialize(Serializer& ar)
{
    ar & m_logicExpression
       & m_questionTexts
       & m_helpTexts;
}
