#pragma once

#include <zCaseO/zCaseO.h>
#include <zCaseO/NumericCaseItem.h>


class ZCASEO_API FixedWidthNumericCaseItem : public NumericCaseItem, public FixedWidthCaseItem
{
    friend class CaseItem;

protected:
    FixedWidthNumericCaseItem(const CDictItem& dict_item, Type type = Type::FixedWidthNumeric);

public:
    /// <summary>
    /// Sets the numeric case item based on the text input.
    /// </summary>
    virtual void SetValueFromTextInput(CaseItemIndex& index, const TCHAR* text_value) const
    {
        SetValueFromInput(index, ConvertTextToNumber(text_value));
    }

    /// <summary>
    /// Converts the text to a number using the dictionary properties.
    /// </summary>
    double ConvertTextToNumber(const TCHAR* text_value) const;

    /// <summary>
    /// Converts the number to text using the dictionary properties. The buffer must have
    /// enough space to store the complete length of the number.
    /// </summary>
    void ConvertNumberToText(double value, TCHAR* text_buffer) const;

    void OutputFixedValue(const CaseItemIndex& index, TCHAR* text_buffer) const override
    {
        ConvertNumberToText(GetValueForOutput(index), text_buffer);
    }

private:
    enum class ConvertTextRoutine { Int32, Int64, Double };
    ConvertTextRoutine m_convertTextRoutine;
};
