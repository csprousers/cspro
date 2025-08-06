#include "StdAfx.h"
#include "CapiQuestion.h"


CapiQuestion::CapiQuestion(std::string item_name/* = std::string()*/)
    :   m_itemName(std::move(item_name))
{
}


void CapiQuestion::SetCondition(CapiCondition condition)
{
    auto lookup = std::find_if(m_conditions.begin(), m_conditions.end(),
                               [&](const CapiCondition& c) { return ( condition.GetLogic() == c.GetLogic() ); });

    if( lookup == m_conditions.end() )
    {
        m_conditions.emplace_back(std::move(condition));
    }

    else
    {
        *lookup = std::move(condition);
    }
}


bool CapiQuestion::IsDefined() const
{
    // force the user to delete extra conditions before considering the question undefined
    if( m_conditions.size() != 1 )
        return !m_conditions.empty();

    const CapiCondition& condition = m_conditions.front();

    // if the condition has logic, consider this a defined question even if it has blank question text
    if( !condition.GetLogic().empty() )
        return true;

    auto process = [](const std::map<std::string, CapiText>& texts)
    {
        for( const auto& [language, capi_text] : texts )
        {
            if( !SO::IsWhitespace(capi_text.GetText().GetString()) )
                return true;
        }

        return false;
    };

    return ( process(condition.GetAllQuestionText()) ||
             process(condition.GetAllHelpText()) );
}


void CapiQuestion::WriteJson(JsonWriter& json_writer) const
{
    json_writer.BeginObject();

    auto [dictionary_sv, name_sv] = SO::GetTextOnEitherSideOfCharacter(m_itemName, '.');

    if( name_sv.empty() )
        std::swap(dictionary_sv, name_sv);

    json_writer.Write(JK::name, name_sv)
               .WriteIfNotBlank(JK::dictionary, dictionary_sv)
               .Write(JK::conditions, m_conditions);

    json_writer.EndObject();
}


void CapiQuestion::serialize(Serializer& ar)
{
    ar & m_itemName
       & m_conditions;

    if( ar.PredatesVersionIteration(Serializer::Iteration_8_1_000_1) )
    {
        ASSERT(ar.IsLoading() && m_pre81FillExpressions == nullptr);
        m_pre81FillExpressions = std::make_unique<std::map<std::string, int>>();
        ar >> *m_pre81FillExpressions;
    }
}
