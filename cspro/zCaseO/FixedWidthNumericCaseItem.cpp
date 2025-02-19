#include "stdafx.h"
#include "FixedWidthNumericCaseItem.h"
#include <zToolsO/NumberConverter.h>


FixedWidthNumericCaseItem::FixedWidthNumericCaseItem(const CDictItem& dict_item, const Type type/* = Type::FixedWidthNumeric*/)
    :   NumericCaseItem(dict_item, type, true)
{
    m_convertTextRoutine = ( dict_item.GetDecimal() != 0 ) ? ConvertTextRoutine::Double :
                           ( dict_item.GetLen() <= 9 )     ? ConvertTextRoutine::Int32 :
                                                             ConvertTextRoutine::Int64;
}


void FixedWidthNumericCaseItem::SetValueFromTextInput(CaseItemIndex& index, const TCHAR* const text_value) const
{
    SetValueFromInput(index, ConvertTextToNumber(text_value));
}


double FixedWidthNumericCaseItem::ConvertTextToNumber(const TCHAR* const text_value) const
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


void FixedWidthNumericCaseItem::ConvertNumberToText(const double value, TCHAR* const text_buffer) const
{
    switch( m_convertTextRoutine )
    {
        case ConvertTextRoutine::Int32:
            NumberConverter::IntegerDoubleToText<int>(text_buffer, value, m_dictItem.GetLen(), m_dictItem.GetZeroFill());
            return;

        case ConvertTextRoutine::Int64:
            NumberConverter::IntegerDoubleToText<int64_t>(text_buffer, value, m_dictItem.GetLen(), m_dictItem.GetZeroFill());
            return;

        default:
            ASSERT(m_convertTextRoutine == ConvertTextRoutine::Double);
            NumberConverter::DoubleToText(text_buffer, value, m_dictItem.GetLen(), m_dictItem.GetDecimal(), m_dictItem.GetZeroFill(), m_dictItem.GetDecChar());
            return;
    }
}


size_t FixedWidthNumericCaseItem::GetMaxUtf8FixedValueWidth() const
{
    return m_dictItem.GetLen();
}


size_t FixedWidthNumericCaseItem::OutputFixedValue(const CaseItemIndex& index, char* const text_buffer) const
{
    auto wide_buffer = std::make_unique_for_overwrite<wchar_t[]>(GetMaxUtf8FixedValueWidth());
    ConvertNumberToText(GetValueForOutput(index), wide_buffer.get());
    const std::string utf8_text = UTF8_TODO::GetUtf8(std::wstring_view(wide_buffer.get(), GetMaxUtf8FixedValueWidth()));
    ASSERT(utf8_text.length() == GetMaxUtf8FixedValueWidth());
    memcpy(text_buffer, utf8_text.data(), utf8_text.length());
    return utf8_text.length();
}
