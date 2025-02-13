#pragma once


class CaseItemConverter
{
public:
    virtual ~CaseItemConverter() { }

    virtual bool ToNumber(const NumericCaseItem& output_numeric_case_item, CaseItemIndex& output_index) = 0;
    virtual bool ToString(const StringCaseItem& output_string_case_item, CaseItemIndex& output_index) = 0;
    virtual bool ToBinary(const BinaryCaseItem& output_binary_case_item, CaseItemIndex& output_index) = 0;
};
