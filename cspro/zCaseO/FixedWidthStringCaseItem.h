#pragma once

#include <zCaseO/zCaseO.h>
#include <zCaseO/FixedWidthCaseItem.h>
#include <zCaseO/StringCaseItem.h>


class ZCASEO_API FixedWidthStringCaseItem : public StringCaseItem, public FixedWidthCaseItem
{
    friend class CaseItem;
    friend class FixedWidthNumericWithStringBufferCaseItem;

protected:
    FixedWidthStringCaseItem(const CDictItem& dict_item);

    void AllocateMemory(void* data_buffer) const override;

    void ResetValue(void* data_buffer) const override;

public:
    // Returns true if the string if non-blank.
    bool IsBlank(const CaseItemIndex& index) const override;

    // Sets the string from a buffer that contains the exact number of characters (measured in wide) needed to fill the string.
    void SetFixedWidthValue(CaseItemIndex& index, std::string_view value_sv) const;
    void SetFixedWidthValue(CaseItemIndex& index, const char* value) const;
    void SetFixedWidthValue(CaseItemIndex& index, const TCHAR* value) const;

protected:
    void AdjustValueForType(SharableString& value) const override;

    size_t GetMaxUtf8FixedValueWidth() const override;
    size_t OutputFixedValue(const CaseItemIndex& index, char* text_buffer) const override;
};
