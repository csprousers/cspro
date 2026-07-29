#include "StdAfx.h"
#include "DictValue.h"
#include <zHtml/AccessUrlSerializer.h>


DictValue::DictValue()
    :   m_textColor(DictionaryDefaults::ValueLabelTextColor)
{
}


void DictValue::SetSpecialValue(const double value)
{
    ASSERT(::IsSpecial(value));
    m_specialValue = value;
}


void DictValue::SetSpecialValue(const std::optional<double>& value)
{
    if( value.has_value() )
    {
        SetSpecialValue(*value);
    }

    else
    {
        SetNotSpecial();
    }
}


void DictValue::AddValuePair(DictValuePair dict_value_pair)
{
    m_dictValuePairs.emplace_back(std::move(dict_value_pair));
}


void DictValue::InsertValuePair(const size_t index, DictValuePair dict_value_pair)
{
    ASSERT(index <= m_dictValuePairs.size());
    m_dictValuePairs.insert(m_dictValuePairs.begin() + index, std::move(dict_value_pair));
}


void DictValue::RemoveValuePair(const size_t index)
{
    ASSERT(index < m_dictValuePairs.size());
    m_dictValuePairs.erase(m_dictValuePairs.begin() + index);
}


size_t DictValue::GetNumToValues() const
{
    size_t num_to_values = 0;

    for( const DictValuePair& dict_value_pair : m_dictValuePairs )
    {
        if( !SO::IsBlank(dict_value_pair.GetTo()) )
        {
            ASSERT(!IsSpecial());
            ++num_to_values;
        }
    }

    return num_to_values;
}


std::string DictValue::GetRangeString() const
{
    std::string range_text;

    for( const DictValuePair& dict_value_pair : m_dictValuePairs )
    {
        SO::AppendWithSeparator(range_text, dict_value_pair.GetFrom(), ", ");

        if( !SO::IsBlank(dict_value_pair.GetTo()) )
        {
            range_text.push_back(':');
            range_text.append(dict_value_pair.GetTo());
        }
    }

    return range_text;
}


DictValue DictValue::CreateFromJson(const JsonNode& json_node)
{
    DictValue dict_value;

    dict_value.DictBase::ParseJsonInput(json_node);

    if( json_node.Contains(JK::image) )
        dict_value.m_imageFilePath = json_node.GetAbsolutePath(JK::image);

    dict_value.m_textColor = json_node.GetOrDefault(JK::textColor, DictionaryDefaults::ValueLabelTextColor);

    if( json_node.Contains(JK::special) )
    {
        const std::string_view special_text_sv = json_node.Get<std::string_view>(JK::special);
        dict_value.m_specialValue = SpecialValues::StringIsSpecial<std::optional<double>>(special_text_sv);

        if( !dict_value.m_specialValue.has_value() )
        {
            json_node.LogWarning("The special value text '%s' is not valid",
                                 std::string(special_text_sv).c_str());
        }
    }

    dict_value.m_dictValuePairs = json_node.GetArrayOrEmpty(JK::pairs).GetVector<DictValuePair>(
        [&](const JsonParseException& exception)
        {
            const DictionarySerializerHelper* dict_serializer_helper = json_node.GetSerializerHelper().Get<DictionarySerializerHelper>();

            json_node.LogWarning("A value pair was not added to '%s' due to errors: %s",
                                 ( dict_serializer_helper != nullptr && dict_serializer_helper->GetCurrentValueSet() != nullptr ) ? dict_serializer_helper->GetCurrentValueSet()->GetName().c_str() : "",
                                 exception.what());
        });

    return dict_value;
}


void DictValue::WriteJson(JsonWriter& json_writer) const
{
    json_writer.BeginObject();

    DictBase::WriteJson(json_writer);

    if( json_writer.Verbose() || !m_imageFilePath.empty() )
    {
        json_writer.WriteRelativePath(JK::image, m_imageFilePath);
        AccessUrl::WriteFileAccessUrl(json_writer, m_imageFilePath);
    }

    if( json_writer.Verbose() || m_textColor != DictionaryDefaults::ValueLabelTextColor )
        json_writer.Write(JK::textColor, m_textColor);

    if( m_specialValue.has_value() )
        json_writer.WriteEngineValue(JK::special, *m_specialValue);

    // value pairs (written with tight formatting)
    {
        const JsonWriter::FormattingHolder tight_formatting_holder = json_writer.SetFormattingType(JsonFormattingType::Tight);
        json_writer.Write(JK::pairs, m_dictValuePairs);
    }

    json_writer.EndObject();
}


void DictValue::serialize(Serializer& ar)
{
    DictBase::serialize(ar);

    std::optional<std::string> special_name;

    if( ar.PredatesVersionIteration(Serializer::Iteration_8_0_000_1) )
    {
        special_name = ar.Read<std::string>();
        SetNote(ar.Read<std::string>());
    }

    ar.IgnoreUnusedVariable<std::string>(Serializer::Iteration_8_0_000_1); // m_error

    if( ar.PredatesVersionIteration(Serializer::Iteration_8_0_000_1) )
    {
        bool is_special = ar.Read<bool>();
        enum class ValueType { Value, Missing, NotAppl, Default, Refused };
        ValueType value_type;
        ar.SerializeEnum(value_type);
        ASSERT(is_special == !special_name->empty());
        ASSERT(special_name->empty() == ( value_type == ValueType::Value ));

        if( value_type != ValueType::Value )
        {
            m_specialValue = ( value_type == ValueType::Missing ) ? MISSING :
                             ( value_type == ValueType::NotAppl ) ? NOTAPPL :
                             ( value_type == ValueType::Default ) ? DEFAULT :
                                                                    REFUSED;
        }
    }

    else
    {
        ar & m_specialValue;
    }

    ar.IgnoreUnusedVariable<int>(Serializer::Iteration_8_0_000_1); // m_iNumVPairs

    ar & m_dictValuePairs;

    ar.SerializePath(m_imageFilePath, true);

    ar & m_textColor;
}
