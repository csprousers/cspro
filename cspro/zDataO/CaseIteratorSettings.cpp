#include "stdafx.h"
#include "CaseIteratorSettings.h"
#include "CSWebRepositoryJsonKeys.h"


CREATE_JSON_KEY(viewFilters)
CREATE_JSON_VALUE(startswith)


// --------------------------------------------------------------------------
// CaseIteratorSettings
// --------------------------------------------------------------------------

CaseIteratorSettings::CaseIteratorSettings()
    :   m_status(CaseIterationCaseStatus::NotDeletedOnly)
{
}


CaseIteratorSettings::CaseIteratorSettings(const CaseIteratorSettings& rhs)
    :   m_status(rhs.m_status),
        m_method(rhs.m_method),
        m_order(rhs.m_order),
        m_parameters(CreateCopyOfPointerValue(rhs.m_parameters))
{
}


CaseIteratorSettings& CaseIteratorSettings::operator=(const CaseIteratorSettings& rhs)
{
    m_status = rhs.m_status;
    m_method = rhs.m_method;
    m_order = rhs.m_order;
    m_parameters = CreateCopyOfPointerValue(rhs.m_parameters);

    return *this;
}


void CaseIteratorSettings::ToggleMethod()
{
    m_method = ( GetEvaluatedMethod() == CaseIterationMethod::SequentialOrder ) ? CaseIterationMethod::KeyOrder :
                                                                                  CaseIterationMethod::SequentialOrder;
}


CaseIteratorSettings CaseIteratorSettings::CreateFromJson(const JsonNode& json_node)
{
    const JsonNode sort_json_node = json_node.GetOrEmpty(JK::sort);
    const JsonNode filter_json_node = json_node.GetOrEmpty(JK::filter);

    CaseIteratorSettings settings;

    settings.m_status = json_node.Get<CaseIterationCaseStatus>(JK::status);

    if( !sort_json_node.IsEmpty() )
    {
        settings.m_method = sort_json_node.GetOptional<CaseIterationMethod>(JK::order);

        if( sort_json_node.Contains(JK::ascending) )
        {
            settings.m_order = sort_json_node.Get<bool>(JK::ascending) ? CaseIterationOrder::Ascending :
                                                                         CaseIterationOrder::Descending;
        }
    }

    if( !filter_json_node.IsEmpty() )
    {
        static_assert(static_cast<size_t>(CaseIterationStartType::LessThan) == 0 &&
                      static_cast<size_t>(CaseIterationStartType::GreaterThan) == 3);

        const size_t operator_index = filter_json_node.GetFromStringOptions(JK::operator_, { ToString(CaseIterationStartType::LessThan),
                                                                                             ToString(CaseIterationStartType::LessThanEquals),
                                                                                             ToString(CaseIterationStartType::GreaterThanEquals),
                                                                                             ToString(CaseIterationStartType::GreaterThan),
                                                                                             JV::startswith });

        // startswith
        if( operator_index > static_cast<size_t>(CaseIterationStartType::GreaterThan) )
        {
            ASSERT(filter_json_node.GetOrConstruct<std::string>(JK::type) == JK::key);
            settings.m_parameters = CaseIteratorParameters::CreateForKeyPrefix(filter_json_node.Get<std::string>(JK::value));
        }

        // operators
        else
        {
            auto first_key_or_position = ( filter_json_node.GetFromStringOptions(JK::type, { JK::key, JK::position }) == 0 ) ?
                std::variant<std::string, double>(filter_json_node.Get<std::string>(JK::value)) :
                std::variant<std::string, double>(filter_json_node.Get<double>(JK::value));

            settings.m_parameters = CaseIteratorParameters::CreateForKey(static_cast<CaseIterationStartType>(operator_index),
                                                                         std::move(first_key_or_position));
        }
    }

    return settings;
}


void CaseIteratorSettings::WriteJson(JsonWriter& json_writer, const bool write_to_new_json_object/* = true*/) const
{
    if( write_to_new_json_object )
        json_writer.BeginObject();

    json_writer.Write(JK::status, m_status);

    if( m_method.has_value() || m_order.has_value() )
    {
        json_writer.BeginObject(JK::sort)
                   .WriteIfHasValue(JK::order, m_method);

        if( m_order.has_value() )
            json_writer.Write(JK::ascending, ( *m_order == CaseIterationOrder::Ascending ));

        json_writer.EndObject();
    }

    if( m_parameters != nullptr )
    {
        json_writer.BeginObject(JK::filter);

        // use the key prefix if it is set and and is not empty
        if( m_parameters->key_prefix.has_value() && !m_parameters->key_prefix->empty() )
        {
            json_writer.Write(JK::operator_, JV::startswith)
                       .Write(JK::type, JK::key)
                       .Write(JK::value, *m_parameters->key_prefix);
        }

        else
        {
            json_writer.Write(JK::operator_, ToString(m_parameters->start_type))
                       .Write(JK::type, std::holds_alternative<std::string>(m_parameters->first_key_or_position) ? JK::key : JK::position)
                       .WriteVariant(JK::value, m_parameters->first_key_or_position);
        }

        json_writer.EndObject();
    }

    if( write_to_new_json_object )
        json_writer.EndObject();
}



// --------------------------------------------------------------------------
// ViewableCaseIteratorSettings
// --------------------------------------------------------------------------

ViewableCaseIteratorSettings::ViewableCaseIteratorSettings()
    :   m_viewCaseLabel(true),
        m_viewFilters(false)
{
}


ViewableCaseIteratorSettings::ViewableCaseIteratorSettings(CaseIteratorSettings settings)
    :   CaseIteratorSettings(std::move(settings)),
        m_viewCaseLabel(false),
        m_viewFilters(false)
{
}


ViewableCaseIteratorSettings ViewableCaseIteratorSettings::CreateFromJson(const JsonNode& json_node)
{
    ViewableCaseIteratorSettings settings(CaseIteratorSettings::CreateFromJson(json_node));

    if( json_node.Contains(JK::view) )
        settings.m_viewCaseLabel = ( json_node.GetFromStringOptions(JK::view, { JK::label, JK::key }) == 0 );

    settings.m_viewFilters = json_node.GetOrDefault(JK::viewFilters, settings.m_viewFilters);

    return settings;
}


void ViewableCaseIteratorSettings::WriteJson(JsonWriter& json_writer) const
{
    json_writer.BeginObject();

    CaseIteratorSettings::WriteJson(json_writer, false);

    json_writer.Write(JK::view, m_viewCaseLabel ? JK::label : JK::key)
               .Write(JK::viewFilters, m_viewFilters);

    json_writer.EndObject();
}
