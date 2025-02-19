#pragma once

#include <zCaseO/zCaseO.h>

class BinaryCaseItem;
class CaseItem;
class CaseItemIndex;
class NumericCaseItem;
class StringCaseItem;
class ValueProcessor;


class ZCASEO_API CaseItemPrinter
{
public:
    enum class Format { Label, Code, LabelCode, CodeLabel, CaseTree };

    CaseItemPrinter(Format format);

    Format GetFormat() const      { return m_format; }
    void SetFormat(Format format) { m_format = format; }

    std::string GetText(const CaseItem& case_item, const CaseItemIndex& index) const;

    static std::string FormatNumber(const CDictItem& dict_item, double value, bool case_tree_format);

private:
    template<typename T>
    std::string GetLabel(const CaseItem& case_item, const T& value) const;

    bool UseLabel() const { return ( m_format != Format::Code ); }
    bool UseCode() const { return ( m_format != Format::Label ); }

    void GetText(const NumericCaseItem& numeric_case_item, const CaseItemIndex& index, std::string& label, std::string& code) const;
    void GetText(const StringCaseItem& string_case_item, const CaseItemIndex& index, std::string& label, std::string& code) const;
    void GetText(const BinaryCaseItem& binary_case_item, const CaseItemIndex& index, std::string& label, std::string& code) const;

    std::string FormatNumber(const CaseItem& case_item, double value) const;

private:
    Format m_format;
    mutable std::map<const CaseItem*, std::shared_ptr<const ValueProcessor>> m_valueProcessors;
};
