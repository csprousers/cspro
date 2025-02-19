#pragma once

#include <zReformatO/CaseItemConverter.h>


class NumericCaseItemConverter : public CaseItemConverter
{
public:
    NumericCaseItemConverter(const NumericCaseItem& input_numeric_case_item, const CaseItemIndex& input_index);

    bool ToNumber(const NumericCaseItem& output_numeric_case_item, CaseItemIndex& output_index) override;
    bool ToString(const StringCaseItem& output_string_case_item, CaseItemIndex& output_index) override;
    bool ToBinary(const BinaryCaseItem& output_binary_case_item, CaseItemIndex& output_index) override;

private:
    const NumericCaseItem& m_inputNumericCaseItem;
    const CaseItemIndex& m_inputIndex;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline NumericCaseItemConverter::NumericCaseItemConverter(const NumericCaseItem& input_numeric_case_item, const CaseItemIndex& input_index)
    :   m_inputNumericCaseItem(input_numeric_case_item),
        m_inputIndex(input_index)
{
}


inline bool NumericCaseItemConverter::ToNumber(const NumericCaseItem& output_numeric_case_item, CaseItemIndex& output_index)
{
    output_numeric_case_item.SetValue(output_index, m_inputNumericCaseItem.GetValue(m_inputIndex));
    return true;
}


inline bool NumericCaseItemConverter::ToString(const StringCaseItem& output_string_case_item, CaseItemIndex& output_index)
{
    ASSERT(m_inputNumericCaseItem.IsFixedWidth());
    const FixedWidthNumericCaseItem& fixed_width_numeric_case_item = assert_cast<const FixedWidthNumericCaseItem&>(m_inputNumericCaseItem);

    std::string string_value(fixed_width_numeric_case_item.GetMaxUtf8FixedValueWidth(), '\0');
    const size_t string_length = fixed_width_numeric_case_item.OutputFixedValue(m_inputIndex, string_value.data());
    string_value.resize(string_length);

    output_string_case_item.SetValue(output_index, std::move(string_value));

    return true;
}


inline bool NumericCaseItemConverter::ToBinary(const BinaryCaseItem& /*output_binary_case_item*/, CaseItemIndex& /*output_index*/)
{
    // BINARY_TYPES_TO_ENGINE_TODO for 8.1, potentially support numeric -> Document
    return false;
}
