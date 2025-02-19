#pragma once

#include <zCaseO/zCaseO.h>
#include <zCaseO/FixedWidthCaseItem.h>
#include <zCaseO/NumericCaseItem.h>


class ZCASEO_API FixedWidthNumericCaseItem : public NumericCaseItem, public FixedWidthCaseItem
{
    friend class CaseItem;

protected:
    FixedWidthNumericCaseItem(const CDictItem& dict_item, Type type = Type::FixedWidthNumeric);

public:
    // Sets the numeric value based on the text input.
    virtual void SetValueFromTextInput(CaseItemIndex& index, const TCHAR* text_value) const;

    // Converts the text to a number using the dictionary properties.
    double ConvertTextToNumber(const TCHAR* text_value) const;

    // Converts the number to text using the dictionary properties.
    // The buffer must have enough space to store the complete length of the number.
    void ConvertNumberToText(double value, TCHAR* text_buffer) const;

    size_t GetMaxUtf8FixedValueWidth() const override;
    size_t OutputFixedValue(const CaseItemIndex& index, char* text_buffer) const override;

private:
    enum class ConvertTextRoutine { Int32, Int64, Double };
    ConvertTextRoutine m_convertTextRoutine;
};
