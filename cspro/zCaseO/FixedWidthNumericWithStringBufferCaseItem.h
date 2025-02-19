#pragma once

#include <zCaseO/zCaseO.h>
#include <zCaseO/FixedWidthNumericCaseItem.h>
#include <zCaseO/FixedWidthStringCaseItem.h>


class ZCASEO_API FixedWidthNumericWithStringBufferCaseItem : public FixedWidthNumericCaseItem
{
    friend class CaseItem;

protected:
    FixedWidthNumericWithStringBufferCaseItem(const CDictItem& dict_item);

    size_t GetSizeForMemoryAllocation() const override;
    void AllocateMemory(void* data_buffer) const override;
    void DeallocateMemory(void* data_buffer) const override;

    void ResetValue(void* data_buffer) const override;
    void CopyValue(void* data_buffer, const void* copy_data_buffer) const override;

    size_t StoreBinaryValue(const void* data_buffer, std::byte* binary_buffer) const override;
    void RetrieveBinaryValue(void* data_buffer, const std::byte*& binary_buffer) const override;

public:
    void SetValue(CaseItemIndex& index, double value) const override;
    void SetValueFromTextInput(CaseItemIndex& index, const TCHAR* text_value) const override;
    size_t OutputFixedValue(const CaseItemIndex& index, char* text_buffer) const override;

    // Gets the string representation of the current value.
    const std::string& GetBufferString(const CaseItemIndex& index) const { return *GetBufferSharableString(index); }
    const SharableString& GetBufferSharableString(const CaseItemIndex& index) const;

private:
    const FixedWidthStringCaseItem m_bufferStringCaseItem;

    // CR_TODO the use of the flag m_settingValueFromTextInput is not thread safe
    mutable bool m_settingValueFromTextInput;
};
