#include "StdAfx.h"
#include "CapiQuestion.h"


CapiQuestion::CapiQuestion(const CString& item_name/* = CString()*/)
    :   m_itemName(item_name)
{
}


const CapiCondition* CapiQuestion::GetCondition(const CString& logic, int min_occ, int max_occ) const
{
    auto cond = std::find_if(m_conditions.begin(), m_conditions.end(),
        [&](const CapiCondition& c) { return logic == c.GetLogic() &&
                                      min_occ == c.GetMinOcc() &&
                                      max_occ == c.GetMaxOcc(); });
    if (cond == m_conditions.end()) {
        return nullptr;
    }
    else {
        return &(*cond);
    }
}


void CapiQuestion::SetCondition(CapiCondition condition)
{
    auto match = std::find_if(m_conditions.begin(), m_conditions.end(),
        [&](const CapiCondition& c) { return condition.GetLogic() == c.GetLogic() &&
                                             condition.GetMinOcc() == c.GetMinOcc() &&
                                             condition.GetMaxOcc() == c.GetMaxOcc(); });
    if (match == m_conditions.end()) {
        m_conditions.emplace_back(std::move(condition));
    }
    else {
        *match = std::move(condition);
    }
}


void CapiQuestion::WriteJson(JsonWriter& json_writer) const
{
    json_writer.BeginObject();

    wstring_view item_name_sv = m_itemName;
    size_t dot_pos = item_name_sv.find('.');

    json_writer.Write(JK::name, item_name_sv.substr(dot_pos + 1));

    if( dot_pos != std::wstring_view::npos )
        json_writer.Write(JK::dictionary, item_name_sv.substr(0, dot_pos));

    json_writer.Write(JK::conditions, m_conditions);

    json_writer.EndObject();
}


void CapiQuestion::serialize(Serializer& ar)
{
    ar & m_itemName
       & m_conditions
       & m_fillExpressions;
}
