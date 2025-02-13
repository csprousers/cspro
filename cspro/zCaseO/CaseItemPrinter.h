#pragma once

#include <zCaseO/zCaseO.h>

class CaseItem;
class CaseItemIndex;
class ValueProcessor;


class ZCASEO_API CaseItemPrinter
{
public:
    enum class Format { Label, Code, LabelCode, CodeLabel, CaseTree };

    CaseItemPrinter(Format format);

    void SetFormat(Format format) { m_format = format; }

    std::wstring GetText(const CaseItem& case_item, const CaseItemIndex& index) const;

private:
    template<typename T>
    std::wstring GetLabel(const CaseItem& case_item, const T& value) const;

    std::wstring FormatNumber(const CaseItem& case_item, double value) const;

private:
    Format m_format;
    mutable std::map<const CaseItem*, std::shared_ptr<const ValueProcessor>> m_valueProcessors;
};
