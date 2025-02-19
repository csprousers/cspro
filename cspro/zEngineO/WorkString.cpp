#include "stdafx.h"
#include "WorkString.h"
#include <zJavaScript/Executor.h>


// --------------------------------------------------------------------------
// WorkString
// --------------------------------------------------------------------------

WorkString::WorkString(std::string string_name)
    :   Symbol(std::move(string_name), SymbolType::WorkString)
{
}


WorkString::WorkString(const WorkString& work_string)
    :   Symbol(work_string)
{
}


void WorkString::CompareDeclarationAttributes(const Symbol& symbol) const
{
    const WorkString& work_string = assert_cast<const WorkString&>(symbol);
    const bool this_is_alpha = ( GetSubType() == SymbolSubType::WorkAlpha );

    if( GetSubType() != symbol.GetSubType() )
    {
        throw CompareDeclarationAttributesException("symbol type: %s vs. %s", this_is_alpha ? "alpha" : "string",
                                                                              this_is_alpha ? "string" : "alpha");
    }

    if( this_is_alpha )
    {
        const WorkAlpha& work_alpha1 = assert_cast<const WorkAlpha&>(*this);
        const WorkAlpha& work_alpha2 = assert_cast<const WorkAlpha&>(work_string);

        if( work_alpha1.GetWideLength() != work_alpha2.GetWideLength() )
        {
            throw CompareDeclarationAttributesException("alpha length: %d vs. %d", static_cast<int>(work_alpha1.GetWideLength()),
                                                                                   static_cast<int>(work_alpha2.GetWideLength()));
        }
    }
}


std::unique_ptr<Symbol> WorkString::CloneInInitialState() const
{
    return std::unique_ptr<WorkString>(new WorkString(*this));
}


void WorkString::Reset()
{
    m_string.Reset();
}


void WorkString::WriteValueToJson(JsonWriter& json_writer) const
{
    json_writer.WriteEngineValue(m_string);
}


void WorkString::SetValueFromJson(const JsonNode& json_node)
{
    SetString(json_node.GetEngineValue<SharableString>());
}


JavaScript::Value WorkString::GetJavaScriptValue(JavaScript::Executor& executor) const
{
    return executor.CreateEngineValue(m_string);
}


void WorkString::SetValueFromJavaScript(JavaScript::Executor& executor, const JavaScript::Value& js_value)
{
    SetString(executor.ConvertEngineValue<SharableString>(js_value));
}



// --------------------------------------------------------------------------
// WorkAlpha
// --------------------------------------------------------------------------

WorkAlpha::WorkAlpha(std::string alpha_name)
    :   WorkString(std::move(alpha_name)),
        m_stringInResetState(SO::Empty_shared_string)
{
    SetSubType(SymbolSubType::WorkAlpha);
}


WorkAlpha::WorkAlpha(const WorkAlpha& work_alpha)
    :   WorkString(work_alpha)
{
    SetWideLength(work_alpha.GetWideLength());
}


std::unique_ptr<Symbol> WorkAlpha::CloneInInitialState() const
{
    return std::unique_ptr<WorkString>(new WorkAlpha(*this));
}


void WorkAlpha::SetWideLength(const size_t length)
{
    m_stringInResetState = std::make_shared<const std::string>(length, ' ');
}


void WorkAlpha::Reset()
{
    m_string = m_stringInResetState;
}


void WorkAlpha::SetString(SharableString&& sharable_string)
{
    sharable_string.WideMakeExactLength(GetWideLength());
    m_string = std::move(sharable_string);
}


void WorkAlpha::serialize_subclass(Serializer& ar)
{
    if( ar.IsSaving() )
    {
        ar.Write(static_cast<unsigned>(GetWideLength()));
    }

    else
    {
        SetWideLength(ar.Read<unsigned>());
    }
}


void WorkAlpha::WriteJsonMetadata_subclass(JsonWriter& json_writer) const
{
    json_writer.Write(JK::subtype, GetSubType());
    json_writer.Write(JK::length, GetWideLength());
}
