#include "StdAfx.h"
#include "ValueSetResponse.h"
#include <zToolsO/VarFuncs.h>


ValueSetResponse::ValueSetResponse(const CDictItem& dict_item, const DictValue& dict_value, const DictValuePair& dict_value_pair)
    :   m_dictValue(&dict_value),
        m_minValue(DEFAULT)
{
    if( dict_item.GetContentType() == ContentType::Alpha )
    {
        m_code = CString(SO::TrimRight(dict_value_pair.GetFrom()));
    }

    else if( dict_item.GetContentType() == ContentType::Numeric )
    {
        // if the from value is numeric, use it
        if( CIMSAString::IsNumeric(dict_value_pair.GetFrom()) )
        {
            m_minValue = atod(dict_value_pair.GetFrom());
            m_code = WS2CS(FormatValueForDisplay(dict_item, m_minValue));
        }

        // if a special value, use the special value instead
        if( dict_value.IsSpecial() )
        {
            m_minValue = dict_value.GetSpecialValue();

            // but use the formatted code if it was defined above
            if( m_code.IsEmpty() )
                m_code = dict_value_pair.GetFrom();
        }

        // see if the value is a range
        else if( CIMSAString::IsNumeric(dict_value_pair.GetTo()) )
        {
            double max_value = atod(dict_value_pair.GetTo());

            if( max_value != m_minValue )
            {
                m_maxValue = max_value;
                m_code.Append(_T(" - ") + WS2CS(FormatValueForDisplay(dict_item, max_value)));
            }
        }
    }

    else
    {
        CONTENT_TYPE_REFACTOR::LOOK_AT("what to do about non-numeric + non-alpha fields?");
    }
}


ValueSetResponse::ValueSetResponse(const CString& label, double value)
    :   m_dictValue(nullptr),
        m_code(label),
        m_minValue(value)
{
    // this constructor is used to add Not Applicable (from CanEnterNotAppl) to a value set
    m_nonDictValues = std::make_unique<std::tuple<CString, CString, PortableColor>>(label, CString(), DictionaryDefaults::ValueLabelTextColor);
}


double ValueSetResponse::GetMaximumValue() const
{
    return IsDiscrete() ? DEFAULT : *m_maxValue;
}


std::wstring ValueSetResponse::FormatValueForDisplay(const CDictItem& dict_item, double value)
{
    if( IsSpecial(value) )
    {
        return ( value == NOTAPPL ) ? std::wstring() : 
                                      SpecialValues::ValueToString(value);
    }

    else
    {
        // format the value (without zero fill but with a decimal character)
        std::wstring string_value = dvaltochar(value, dict_item.GetCompleteLen(), dict_item.GetDecimal(), false, true);
        SO::MakeTrim(string_value);

        if( dict_item.GetDecimal() > 0 )
        {
            // right-trim decimal zeros
            SO::MakeTrimRight(string_value, '0');

            // make sure that there is at least one value after the decimal mark
            if( string_value.back() == '.' )
                string_value.push_back('0');

            // don't allow strings to start with a decimal mark
            if( string_value.front() == '.' )
            {
                string_value.insert(string_value.begin(), '0');
            }

            else if( string_value.front() == '-' && string_value[1] == '.' )
            {
                string_value.insert(string_value.begin() + 1, '0');
            }
        }

        return string_value;
    }    
}
