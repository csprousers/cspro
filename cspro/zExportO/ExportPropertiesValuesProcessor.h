#pragma once


class ExportPropertiesValuesProcessor
{
public:
    ExportPropertiesValuesProcessor(const ConnectionString& connection_string);

    template<typename PC>
    void Process(const CDictItem& dict_item, PC process_callback);

private:
    bool m_writeCodes;
    bool m_writeLabels;

    bool m_headerForceNames;
    bool m_headerForceLabels;
};



inline ExportPropertiesValuesProcessor::ExportPropertiesValuesProcessor(const ConnectionString& connection_string)
{
    // process whether to write codes and/or labels
    m_writeCodes = connection_string.HasPropertyOrDefault(CSProperty::writeCodes, CSValue::true_, true);
    m_writeLabels = connection_string.HasProperty(CSProperty::writeLabels, CSValue::true_, true);

    const std::wstring* const values_property_pre80 = connection_string.GetProperty(_T("values"));

    if( values_property_pre80 != nullptr )
    {
        if( SO::EqualsNoCase(*values_property_pre80, CSValue::codes) )
        {
            m_writeCodes = true;
            m_writeLabels = false;
        }

        else if( SO::EqualsNoCase(*values_property_pre80, CSValue::labels) )
        {
            m_writeCodes = false;
            m_writeLabels = true;
        }

        else if( SO::EqualsNoCase(*values_property_pre80, _T("codes-and-labels")) )
        {
            m_writeCodes = true;
            m_writeLabels = true;
        }
    }

    if( !m_writeCodes && !m_writeLabels )
    {
        throw CSProException(_T("You cannot suppress writing both codes and labels when writing a file of type '%s'."),
                             ToString(connection_string.GetType()));
    }

    // process how to write the header
    m_headerForceNames = connection_string.HasProperty(CSProperty::header, CSValue::names);
    m_headerForceLabels = connection_string.HasProperty(CSProperty::header, CSValue::labels);
}


template<typename PC>
void ExportPropertiesValuesProcessor::Process(const CDictItem& dict_item, PC process_callback)
{
    // add columns for codes and/or labels
    std::shared_ptr<const ValueProcessor> value_processor;
                
    if( m_writeLabels && dict_item.HasValueSets() )
        value_processor = ValueProcessor::CreateValueProcessor(dict_item, &dict_item.GetValueSet(0));

    // add codes when outputting codes or when there are no value set labels to use
    if( m_writeCodes || value_processor == nullptr )
    {
        // if outputting codes and labels, the default setting will use names
        // for the codes header and labels for the labels header
        const bool use_label_for_header = ( m_headerForceLabels || ( !m_headerForceNames && value_processor == nullptr ) );

        process_callback(nullptr, use_label_for_header);
    }

    // add labels 
    if( value_processor != nullptr )
        process_callback(std::move(value_processor), !m_headerForceNames);
}
