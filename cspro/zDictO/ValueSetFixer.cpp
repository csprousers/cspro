#include "StdAfx.h"
#include "ValueSetFixer.h"


std::optional<bool> ValueSetFixer::m_zeroBeforeDecimal;


ValueSetFixer::ValueSetFixer(const CDictItem& dict_item, TCHAR decimal_char/* = 0*/)
    :   m_dictItem(dict_item),
        m_decimalChar(decimal_char)
{
}


void ValueSetFixer::Fix(std::vector<DictValueSet>& dict_value_sets)
{
    if( !IsNumeric(m_dictItem) )
        return;

    for( DictValueSet& dict_value_set : dict_value_sets )
        Fix(dict_value_set);
}


void ValueSetFixer::Fix(DictValueSet& dict_value_set)
{
    if( !IsNumeric(m_dictItem) )
        return;

    for( DictValue& dict_value : dict_value_set.GetValues() )
        Fix(dict_value);
}


void ValueSetFixer::Fix(DictValue& dict_value)
{
    if( !IsNumeric(m_dictItem) )
        return;

    for( DictValuePair& dict_value_pair : dict_value.GetValuePairs() )
    {
        // notappl values must be blank
        if( dict_value.IsSpecialValue(NOTAPPL) )
        {
            if( dict_value_pair.GetFrom().GetLength() != (int)m_dictItem.GetLen() )
                dict_value_pair.SetFrom(CString(' ', (int)m_dictItem.GetLen()));

            ASSERT(dict_value_pair.GetTo().IsEmpty());
        }

        else
        {
            Fix(dict_value_pair);
        }
    }
}


void ValueSetFixer::Fix(DictValuePair& dict_value_pair)
{
    if( !IsNumeric(m_dictItem) )
        return;

    // formerly CDictItem::FixNumericVSetValues...
    //   "Turn the vset from and to values from string to double and back to string again
    //    Seems like this is done to fix the decimal point/comma if the dictionary file
    //    locale is different from the local the machine that is reading the file."


    // get the default decimal character
    if( m_decimalChar == 0 )
        m_decimalChar = CIMSAString::GetDecChar();

    // get whether or not a zero should appear before the decimal mark
    if( !m_zeroBeforeDecimal.has_value() )
    {
#ifdef WIN32
        m_zeroBeforeDecimal = ( GetPrivateProfileInt(_T("intl"), _T("iLZero"), 0, _T("WIN.INI")) == 1 );
#else
        m_zeroBeforeDecimal = true;
#endif
    }

    if( m_dtoaSpace == nullptr )
        m_dtoaSpace = std::make_unique<TCHAR[]>(30);

    unsigned length_without_decimal_char = m_dictItem.GetLen();

    if( m_dictItem.GetDecimal() > 0 && m_dictItem.GetDecChar() )
        --length_without_decimal_char;

    auto fix = [&](const CString& text) -> const TCHAR*
    {
        double value = atod(text);

        if( value == IMSA_BAD_DOUBLE )
            return text;

        bool evaluated_zero_before_decimal = *m_zeroBeforeDecimal;

        if( ( evaluated_zero_before_decimal && m_dictItem.GetDecimal() > 0 ) &&
            ( ( length_without_decimal_char == m_dictItem.GetDecimal() ) ||
              ( length_without_decimal_char == ( m_dictItem.GetDecimal() + 1 ) && value < 0 ) ) )
        {
            evaluated_zero_before_decimal = false;
        }

        return dtoa(value, m_dtoaSpace.get(), m_dictItem.GetDecimal(), m_decimalChar, evaluated_zero_before_decimal);
    };

    if( !SO::IsWhitespace(dict_value_pair.GetFrom()) )
        dict_value_pair.SetFrom(fix(dict_value_pair.GetFrom()));

    if( !SO::IsWhitespace(dict_value_pair.GetTo()) )
        dict_value_pair.SetTo(fix(dict_value_pair.GetTo()));
}
