#pragma once

#include <zReformatO/CaseItemConverter.h>


class StringCaseItemConverter : public CaseItemConverter
{
public:
    StringCaseItemConverter(const StringCaseItem& input_string_case_item, const CaseItemIndex& input_index);

    bool ToNumber(const NumericCaseItem& output_numeric_case_item, CaseItemIndex& output_index) override;
    bool ToString(const StringCaseItem& output_string_case_item, CaseItemIndex& output_index) override;
    bool ToBinary(const BinaryCaseItem& output_binary_case_item, CaseItemIndex& output_index) override;

private:
    const SharableString m_value;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline StringCaseItemConverter::StringCaseItemConverter(const StringCaseItem& input_string_case_item, const CaseItemIndex& input_index)
    :   m_value(input_string_case_item.GetSharableString(input_index))
{
}


inline bool StringCaseItemConverter::ToNumber(const NumericCaseItem& output_numeric_case_item, CaseItemIndex& output_index)
{
    ASSERT(output_numeric_case_item.IsFixedWidth());

    const FixedWidthNumericCaseItem& output_fixed_width_numeric_case_item = assert_cast<const FixedWidthNumericCaseItem&>(output_numeric_case_item);
    const size_t item_length = output_fixed_width_numeric_case_item.GetDictItem().GetLen();

    if( item_length == m_value.WideLength() )
    {
        output_fixed_width_numeric_case_item.SetValueFromTextInput(output_index, UTF8_TODO::GetWide(*m_value).c_str());
    }

    else
    {
        std::wstring resized_value = UTF8_TODO::GetWide(*m_value);
        SO::MakeExactLength(resized_value, item_length);
        output_fixed_width_numeric_case_item.SetValueFromTextInput(output_index, resized_value.c_str());
    }

    return true;
}


inline bool StringCaseItemConverter::ToString(const StringCaseItem& output_string_case_item, CaseItemIndex& output_index)
{
    output_string_case_item.SetValue(output_index, m_value);
    return true;
}


inline bool StringCaseItemConverter::ToBinary(const BinaryCaseItem& /*output_binary_case_item*/, CaseItemIndex& /*output_index*/)
{
    // BINARY_TYPES_TO_ENGINE_TODO for 8.1, potentially support string -> Document
    return false;
}
