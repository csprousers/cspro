#include "StdAfx.h"
#include "PropertyString.h"
#include <zToolsO/Encoders.h>


void PropertyString::InitializeFromString(const wstring_view property_string_text_sv)
{
    // parse the properties
    wstring_view main_value_sv;
    wstring_view properties_text_sv;
    const size_t pipe_pos = property_string_text_sv.find('|');

    if( pipe_pos != wstring_view::npos )
    {
        main_value_sv = property_string_text_sv.substr(0, pipe_pos);
        properties_text_sv = property_string_text_sv.substr(pipe_pos + 1);
    }

    else
    {
        main_value_sv = property_string_text_sv;
    }

    main_value_sv = SO::Trim(main_value_sv);
    SetMainValue(main_value_sv);

    if( properties_text_sv.empty() )
        return;

    for( wstring_view attribute_sv : SO::SplitString<wstring_view>(properties_text_sv, '&') )
    {
        std::wstring value;
        const size_t equals_pos = attribute_sv.find('=');

        if( equals_pos != std::wstring::npos )
        {
            value = Encoders::FromPercentEncoding(SO::TrimLeft(attribute_sv.substr(equals_pos + 1)));
            attribute_sv = SO::TrimRight(attribute_sv.substr(0, equals_pos));
        }

        if( !PreprocessProperty(attribute_sv, value) )
            m_properties.emplace_back(attribute_sv, std::move(value));
    }
}


const std::wstring* PropertyString::GetProperty(const wstring_view attribute_sv) const
{
    const auto& lookup = std::find_if(m_properties.cbegin(), m_properties.cend(),
                                      [&](const auto& av) { return SO::EqualsNoCase(std::get<0>(av), attribute_sv); });

    return ( lookup != m_properties.cend() ) ? &std::get<1>(*lookup) :
                                               nullptr;
}


void PropertyString::SetProperty(const wstring_view attribute_sv, std::wstring value)
{
    auto lookup = std::find_if(m_properties.begin(), m_properties.end(),
                               [&](const auto& av) { return SO::EqualsNoCase(std::get<0>(av), attribute_sv); });

    if( lookup != m_properties.end() )
    {
        std::get<1>(*lookup) = std::move(value);
    }

    else
    {
        m_properties.emplace_back(attribute_sv, std::move(value));
    }
}


void PropertyString::SetProperty(const wstring_view attribute_sv, double value)
{
    SetProperty(attribute_sv, DoubleToString(value));
}


void PropertyString::SetOrClearProperty(const wstring_view attribute_sv, std::wstring value)
{
    if( value.empty() )
    {
        ClearProperty(attribute_sv);
    }

    else
    {
        SetProperty(attribute_sv, std::move(value));
    }
}


void PropertyString::ClearProperty(const wstring_view attribute_sv)
{
    auto lookup = std::find_if(m_properties.begin(), m_properties.end(),
        [&](const auto& av) { return SO::EqualsNoCase(std::get<0>(av), attribute_sv); });

    if( lookup != m_properties.end() )
        m_properties.erase(lookup);
}


std::wstring PropertyString::ToString(std::wstring main_value, const std::vector<std::tuple<std::wstring, std::wstring>>& properties) const
{
    std::wstring& property_string = main_value;

    bool added_property = false;

    for( const auto& [attribute, value] : properties )
    {
        property_string.push_back(added_property ? '&' : '|');
        property_string.append(attribute);

        if( !value.empty() )
        {
            property_string.push_back('=');
            property_string.append(Encoders::ToPercentEncoding(value));
        }

        added_property = true;
    };

    return property_string;
}


void PropertyString::WriteJson(JsonWriter& json_writer, const bool write_to_new_json_object/* = true*/) const
{
    if( write_to_new_json_object )
        json_writer.BeginObject();

    for( const auto& [attribute, value] : m_properties )
    {
        if( !value.empty() )
        {
            json_writer.Write(attribute, value);
        }

        else
        {
            json_writer.WriteNull(attribute);
        }
    }

    if( write_to_new_json_object )
        json_writer.EndObject();
}
