#include "StdAfx.h"
#include "CapiQuestion.h"


CapiQuestion::CapiQuestion(std::string item_name/* = std::string()*/)
    :   m_itemName(std::move(item_name))
{
}


const CapiCondition* CapiQuestion::GetCondition(const std::string& logic, const int min_occ, const int max_occ) const
{
    const auto& lookup = std::find_if(m_conditions.cbegin(), m_conditions.cend(),
        [&](const CapiCondition& condition)
        {
            return ( logic == condition.GetLogic() &&
                     min_occ == condition.GetMinOcc() &&
                     max_occ == condition.GetMaxOcc() );
        });

    return ( lookup != m_conditions.cend() ) ? &(*lookup) :
                                               nullptr;
}


void CapiQuestion::SetCondition(CapiCondition condition)
{
    auto lookup = std::find_if(m_conditions.begin(), m_conditions.end(),
        [&](const CapiCondition& c)
        {
            return ( condition.GetLogic() == c.GetLogic() &&
                     condition.GetMinOcc() == c.GetMinOcc() &&
                     condition.GetMaxOcc() == c.GetMaxOcc() );
        });

    if( lookup == m_conditions.end() )
    {
        m_conditions.emplace_back(std::move(condition));
    }

    else
    {
        *lookup = std::move(condition);
    }
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
       & m_conditions
       & m_fillExpressions;
}
