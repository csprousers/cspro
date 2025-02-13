#include "stdafx.h"
#include "FixedWidthNumericCaseItem.h"
#include <zToolsO/NumberConverter.h>


FixedWidthNumericCaseItem::FixedWidthNumericCaseItem(const CDictItem& dict_item, Type type/* = Type::FixedWidthNumeric*/)
    :   NumericCaseItem(dict_item, type)
{
    m_convertTextRoutine = ( dict_item.GetDecimal() != 0 ) ? ConvertTextRoutine::Double :
                           ( dict_item.GetLen() <= 9 )     ? ConvertTextRoutine::Int32 :
                                                             ConvertTextRoutine::Int64;
}


double FixedWidthNumericCaseItem::ConvertTextToNumber(const TCHAR* text_value) const
{
    switch( m_convertTextRoutine )
    {
        case ConvertTextRoutine::Int32:
            return NumberConverter::TextToIntegerDouble<int>(text_value, m_dictItem.GetLen());

        case ConvertTextRoutine::Int64:
            return NumberConverter::TextToIntegerDouble<int64_t>(text_value, m_dictItem.GetLen());

        default:
            ASSERT(m_convertTextRoutine == ConvertTextRoutine::Double);
            return NumberConverter::TextToDouble(text_value, m_dictItem.GetLen(), m_dictItem.GetDecimal());
    }
}


void FixedWidthNumericCaseItem::ConvertNumberToText(double value, TCHAR* text_buffer) const
{
    switch( m_convertTextRoutine )
    {
        case ConvertTextRoutine::Int32:
            NumberConverter::IntegerDoubleToText<int>(value, text_buffer, m_dictItem.GetLen(), m_dictItem.GetZeroFill());
            return;

        case ConvertTextRoutine::Int64:
            NumberConverter::IntegerDoubleToText<int64_t>(value, text_buffer, m_dictItem.GetLen(), m_dictItem.GetZeroFill());
            return;

        default:
            ASSERT(m_convertTextRoutine == ConvertTextRoutine::Double);
            NumberConverter::DoubleToText(value, text_buffer, m_dictItem.GetLen(), m_dictItem.GetDecimal(), m_dictItem.GetZeroFill(), m_dictItem.GetDecChar());
            return;
    }
}
