#include "stdafx.h"
#include "WorkString.h"


// --------------------------------------------------------------------------
// WorkString
// --------------------------------------------------------------------------

WorkString::WorkString(std::wstring string_name)
    :   Symbol(std::move(string_name), SymbolType::WorkString)
{
}


WorkString::WorkString(const WorkString& work_string)
    :   Symbol(work_string)
{
}


std::unique_ptr<Symbol> WorkString::CloneInInitialState() const
{
    return std::unique_ptr<WorkString>(new WorkString(*this));
}


void WorkString::Reset()
{
    m_string.clear();
}


void WorkString::WriteValueToJson(JsonWriter& json_writer) const
{
    json_writer.WriteEngineValue(m_string);
}


void WorkString::UpdateValueFromJson(const JsonNode<wchar_t>& json_node)
{
    SetString(json_node.GetEngineValue<std::wstring>());
}



// --------------------------------------------------------------------------
// WorkAlpha
// --------------------------------------------------------------------------

WorkAlpha::WorkAlpha(std::wstring alpha_name)
    :   WorkString(std::move(alpha_name)),
        m_length(0),
        m_numberRightSpaces(0)
{
    SetSubType(SymbolSubType::WorkAlpha);
}


WorkAlpha::WorkAlpha(const WorkAlpha& work_alpha)
    :   WorkString(work_alpha)
{
    SetLength(work_alpha.m_length);
}


std::unique_ptr<Symbol> WorkAlpha::CloneInInitialState() const
{
    return std::unique_ptr<WorkString>(new WorkAlpha(*this));
}


void WorkAlpha::SetLength(const unsigned length)
{
    m_length = length;
    m_numberRightSpaces = m_length;
    SO::MakeExactLength(m_string, m_length);
}


void WorkAlpha::SetString(std::wstring value)
{
    ASSERT80(m_string.length() == m_length);

    const int spaces_needed = m_length - static_cast<int>(value.length());

    if( spaces_needed > 0 )
    {
        _tmemcpy(m_string.data(), value.data(), value.length());

        // only pad the string as needed
        const int actual_spaces_needed = spaces_needed - m_numberRightSpaces;

        if( actual_spaces_needed > 0 )
            _tmemset(m_string.data() + value.length(), ' ', actual_spaces_needed);

        m_numberRightSpaces = spaces_needed;
    }

    else
    {
        m_string = std::move(value);

        if( spaces_needed != 0 )
            m_string.resize(m_length);

        m_numberRightSpaces = 0;
    }

    ASSERT80(m_string.length() == m_length);
}


void WorkAlpha::Reset()
{
    ASSERT80(m_string.length() == m_length);

    if( m_length != m_numberRightSpaces )
    {
        _tmemset(m_string.data(), ' ', m_length - m_numberRightSpaces);
        m_numberRightSpaces = m_length;
    }
}


void WorkAlpha::serialize_subclass(Serializer& ar)
{
    if( ar.IsSaving() )
    {
        ar << m_length;
    }

    else
    {
        SetLength(ar.Read<unsigned>());
    }
}


void WorkAlpha::WriteJsonMetadata_subclass(JsonWriter& json_writer) const
{
    json_writer.Write(JK::subtype, GetSubType());
    json_writer.Write(JK::length, m_length);
}
