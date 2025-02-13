#include "StdAfx.h"
#include "DictBase.h"


DictBase::DictBase() noexcept // DD_STD_REFACTOR_TODO is this needed?
{
}

DictBase::DictBase(const DictBase& rhs) noexcept // DD_STD_REFACTOR_TODO is this needed?
    :   m_label(rhs.m_label),
        m_note(rhs.m_note)
{
}

DictBase::DictBase(DictBase&& rhs) noexcept // DD_STD_REFACTOR_TODO is this needed?
    :   m_label(std::move(rhs.m_label)),
        m_note(std::move(rhs.m_note))
{
}


DictBase& DictBase::operator=(const DictBase& rhs) noexcept // DD_STD_REFACTOR_TODO is this needed?
{
    m_label = rhs.m_label;
    m_note = rhs.m_note;
    return *this;
}

DictBase& DictBase::operator=(DictBase&& rhs) noexcept // DD_STD_REFACTOR_TODO is this needed?
{
    m_label = std::move(rhs.m_label);
    m_note = std::move(rhs.m_note);
    return *this;
}


void DictBase::ParseJsonInput(const JsonNode<wchar_t>& json_node)
{
    m_label = json_node.GetOrDefault(JK::labels, LabelSet::DefaultValue);
    m_note = json_node.GetOrDefault(JK::note, SO::EmptyCString);
}


void DictBase::WriteJson(JsonWriter& json_writer) const
{
    json_writer.Write(JK::labels, m_label)
               .WriteIfNotBlank(JK::note, m_note);
}


void DictBase::serialize(Serializer& ar)
{
    ar & m_label;

    if( ar.MeetsVersionIteration(Serializer::Iteration_8_0_000_1) )
       ar & m_note;
}
