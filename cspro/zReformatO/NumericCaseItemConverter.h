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
    const double m_value;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline NumericCaseItemConverter::NumericCaseItemConverter(const NumericCaseItem& input_numeric_case_item, const CaseItemIndex& input_index)
    :   m_inputNumericCaseItem(input_numeric_case_item),
        m_inputIndex(input_index),
        m_value(m_inputNumericCaseItem.GetValue(m_inputIndex))
{    
}


inline bool NumericCaseItemConverter::ToNumber(const NumericCaseItem& output_numeric_case_item, CaseItemIndex& output_index)
{
    output_numeric_case_item.SetValue(output_index, m_value);
    return true;
}


inline bool NumericCaseItemConverter::ToString(const StringCaseItem& output_string_case_item, CaseItemIndex& output_index)
{
    ASSERT(m_inputNumericCaseItem.IsTypeFixed());

    const size_t item_length = m_inputNumericCaseItem.GetDictionaryItem().GetLen();
    auto string_value_buffer = std::make_unique_for_overwrite<TCHAR[]>(item_length);

    assert_cast<const FixedWidthNumericCaseItem&>(m_inputNumericCaseItem).OutputFixedValue(m_inputIndex, string_value_buffer.get());

    output_string_case_item.SetValue(output_index, wstring_view(string_value_buffer.get(), item_length));

    return true;
}


inline bool NumericCaseItemConverter::ToBinary(const BinaryCaseItem& /*output_binary_case_item*/, CaseItemIndex& /*output_index*/)
{
    // BINARY_TYPES_TO_ENGINE_TODO for 8.1, potentially support numeric -> Document
    return false;
}
