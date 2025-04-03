#include "StdAfx.h"
#include "CapiCondition.h"


CapiCondition::CapiCondition(std::string logic/* = std::string()*/)
    :   m_logic(std::move(logic)),
        m_programIndex(-1)
{
}


const CapiText* CapiCondition::GetText(const std::string& language_name, const CapiText::Type type) const
{
    const std::map<std::string, CapiText>& texts = ( type == CapiText::Type::Question ) ? m_questionTexts :
                                                                                          m_helpTexts;
    const auto& lookup = texts.find(language_name);

    return ( lookup != texts.cend() ) ? &(lookup->second) :
                                        nullptr;
}


void CapiCondition::SetText(CapiText text, const std::string& language_name, const CapiText::Type type)
{
    std::map<std::string, CapiText>& texts = ( type == CapiText::Type::Question ) ? m_questionTexts :
                                                                                    m_helpTexts;
    auto lookup = texts.find(language_name);

    if( lookup == texts.cend() )
    {
        texts.try_emplace(language_name, std::move(text));
    }

    else
    {
        lookup->second = std::move(text);
    }
}


void CapiCondition::DeleteLanguage(const std::string& language_name)
{
    m_questionTexts.erase(language_name);
    m_helpTexts.erase(language_name);
}


void CapiCondition::ModifyLanguage(const std::string& old_language_name, const std::string& new_language_name)
{
    auto modify = [&](std::map<std::string, CapiText>& texts)
    {
        auto lookup = texts.find(old_language_name);

        if( lookup != texts.end() )
        {
            CapiText capi_text = std::move(lookup->second);
            texts.erase(lookup);
            texts[new_language_name] = std::move(capi_text);
        }
    };

    modify(m_questionTexts);
    modify(m_helpTexts);
}



// --------------------------------------------------------------------------
// serialization
// --------------------------------------------------------------------------

CREATE_ENUM_JSON_SERIALIZER(CapiText::Type,
    { CapiText::Type::Question, "question" },
    { CapiText::Type::Help,     "help" })


void CapiCondition::WriteJson(JsonWriter& json_writer) const
{
    json_writer.BeginObject();

    json_writer.WriteIfNotBlank(JK::logic, m_logic);

    if( json_writer.Verbose() || !m_questionTexts.empty() || !m_helpTexts.empty() )
    {
        json_writer.BeginArray(JK::texts);

        auto write_texts = [&](const CapiText::Type type, const std::map<std::string, CapiText>& texts)
        {
            for( const auto& [language, capi_text] : texts )
            {
                if( SO::IsWhitespace(capi_text.GetText().GetString()) )
                    continue;

                json_writer.BeginObject()
                           .Write(JK::language, language)
                           .Write(JK::type, type)
                           .Write(JK::html, capi_text)
                           .EndObject();
            }
        };

        write_texts(CapiText::Type::Question, m_questionTexts);
        write_texts(CapiText::Type::Help, m_helpTexts);

        json_writer.EndArray();
    }

    json_writer.EndObject();
}


void CapiCondition::serialize(Serializer& ar)
{
    if( ar.MeetsVersionIteration(Serializer::Iteration_8_1_000_1) )
    {
        ar & m_programIndex;
    }

    else
    {
        m_programIndex = ar.Read<std::optional<int>>().value_or(-1);
    }

    ar & m_questionTexts
       & m_helpTexts;
}
