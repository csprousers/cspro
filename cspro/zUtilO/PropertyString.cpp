#include "StdAfx.h"
#include "PropertyString.h"
#include <zToolsO/PropertyRetriever.h>


// --------------------------------------------------------------------------
// PropertyString
// --------------------------------------------------------------------------

void PropertyString::InitializeFromString(const std::string_view property_string_text_sv)
{
    // parse the properties
    const auto [main_value_sv, properties_text_sv] = SO::GetTextOnEitherSideOfCharacter(property_string_text_sv, PropertySeparatorInitial);

    SetMainValue(main_value_sv);

    if( properties_text_sv.empty() )
        return;

    for( std::string_view attribute_sv : SO::SplitString<std::string_view>(properties_text_sv, PropertySeparatorAdditional) )
    {
        std::string value;
        const size_t equals_pos = attribute_sv.find('=');

        if( equals_pos != std::string_view::npos )
        {
            value = Encoders::FromPercentEncoding(SO::TrimLeft(attribute_sv.substr(equals_pos + 1)));
            attribute_sv = SO::TrimRight(attribute_sv.substr(0, equals_pos));
        }

        if( !PreprocessProperty(attribute_sv, value) )
            m_properties.emplace_back(attribute_sv, std::move(value));
    }
}


const std::string* PropertyString::GetProperty(const std::string_view attribute_sv) const
{
    const auto& lookup = std::find_if(m_properties.cbegin(), m_properties.cend(),
                                      [&](const auto& av) { return SO::EqualsNoCase(std::get<0>(av), attribute_sv); });

    return ( lookup != m_properties.cend() ) ? &std::get<1>(*lookup) :
                                               nullptr;
}


bool PropertyString::HasPropertyWithValue(const std::string_view attribute_sv) const
{
    const std::string* const property = GetProperty(attribute_sv);

    return ( property != nullptr &&
             !property->empty() );
}


void PropertyString::SetProperty(const std::string_view attribute_sv, std::string value)
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


void PropertyString::SetProperty(const std::string_view attribute_sv, const double value)
{
    SetProperty(attribute_sv, DoubleToString(value));
}


void PropertyString::SetOrClearProperty(const std::string_view attribute_sv, std::string value)
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


void PropertyString::ClearProperty(const std::string_view attribute_sv)
{
    auto lookup = std::find_if(m_properties.begin(), m_properties.end(),
                               [&](const auto& av) { return SO::EqualsNoCase(std::get<0>(av), attribute_sv); });

    if( lookup != m_properties.end() )
        m_properties.erase(lookup);
}


std::string PropertyString::ToString(std::string main_value, const std::vector<std::tuple<std::string, std::string>>& properties)
{
    std::string& property_string = main_value;

    bool added_property = false;

    for( const auto& [attribute, value] : properties )
    {
        property_string.push_back(added_property ? PropertySeparatorAdditional : PropertySeparatorInitial);
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



// --------------------------------------------------------------------------
// PropertyStringPropertyRetriever
// --------------------------------------------------------------------------

class PropertyStringPropertyRetriever : public PropertyRetriever
{
public:
    PropertyStringPropertyRetriever(const PropertyString& property_string)
        :   m_propertyString(property_string)
    {
    }

    std::optional<std::string> GetProperty(const std::string_view attribute_sv) override
    {
        const std::string* const property = m_propertyString.GetProperty(attribute_sv);

        return ( property != nullptr ) ? std::make_optional(*property) :
                                         std::nullopt;
    }

private:
    const PropertyString& m_propertyString;
};


std::unique_ptr<PropertyRetriever> PropertyString::CreatePropertyRetriever() const
{
    return std::make_unique<PropertyStringPropertyRetriever>(*this);
}
