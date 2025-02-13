#include "stdafx.h"
#include "NamedFrequency.h"
#include <engine/FrequencyDriver.h>


// --------------------------------------------------------------------------
// NamedFrequency
// --------------------------------------------------------------------------

NamedFrequency::NamedFrequency(std::wstring frequency_name)
    :   Symbol(std::move(frequency_name), SymbolType::NamedFrequency),
        m_frequencyDriver(nullptr)
{
}


NamedFrequency::NamedFrequency(const NamedFrequency& named_frequency)
    :   Symbol(named_frequency),
        m_frequencyDriver(named_frequency.m_frequencyDriver)
{
    if( named_frequency.m_frequencyIndex.has_value() )
    {
        ASSERT(m_frequencyDriver != nullptr);
        m_frequencyDriver->CloneFrequencyInInitialState(*this, *named_frequency.m_frequencyIndex);
    }
}


std::unique_ptr<Symbol> NamedFrequency::CloneInInitialState() const
{
    return std::unique_ptr<NamedFrequency>(new NamedFrequency(*this));
}


void NamedFrequency::serialize_subclass(Serializer& ar)
{
    ar & m_frequencyIndex;
}


void NamedFrequency::WriteJsonMetadata_subclass(JsonWriter& json_writer) const
{
    ASSERT(m_frequencyDriver != nullptr);

    if( IsFunctionParameter() )
        return;

    m_frequencyDriver->WriteJsonMetadata_subclass(*this, json_writer);
}


void NamedFrequency::WriteValueToJson(JsonWriter& json_writer) const
{
    ASSERT(m_frequencyDriver != nullptr);

    if( IsFunctionParameter() )
    {
        ASSERT(false);
        return;
    }

    m_frequencyDriver->WriteValueToJson(*this, json_writer);
}
