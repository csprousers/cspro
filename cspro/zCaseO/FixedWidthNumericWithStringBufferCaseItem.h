#pragma once

#include <zCaseO/zCaseO.h>
#include <zCaseO/FixedWidthNumericCaseItem.h>


class ZCASEO_API FixedWidthNumericWithStringBufferCaseItem : public FixedWidthNumericCaseItem
{
    friend class CaseItem;

protected:
    FixedWidthNumericWithStringBufferCaseItem(const CDictItem& dict_item);

    size_t GetSizeForMemoryAllocation() const override;
    void AllocateMemory(void* data_buffer) const override;
    void DeallocateMemory(void* data_buffer) const override;

    void ResetValue(void* data_buffer) const override
    {
        _tmemset(GetStringBufferLocation(data_buffer), _T(' '), m_dictItem.GetLen());
        FixedWidthNumericCaseItem::ResetValue(data_buffer);
    }

    void CopyValue(void* data_buffer, const void* copy_data_buffer) const override
    {
        _tmemcpy(GetStringBufferLocation(data_buffer), GetStringBufferLocation(copy_data_buffer), m_dictItem.GetLen());
        FixedWidthNumericCaseItem::CopyValue(data_buffer, copy_data_buffer);
    }

    size_t StoreBinaryValue(const void* data_buffer, std::byte* binary_buffer) const override;
    size_t RetrieveBinaryValue(void* data_buffer, const std::byte* binary_buffer) const override;

public:
    void SetValue(CaseItemIndex& index, double value) const override
    {
        if( !m_settingValueFromTextInput )
            FixedWidthNumericCaseItem::ConvertNumberToText(GetValueForOutput(value), GetStringBufferLocation(index));

        FixedWidthNumericCaseItem::SetValue(index, value);
    }

    void SetValueFromTextInput(CaseItemIndex& index, const TCHAR* text_value) const override
    {
        _tmemcpy(GetStringBufferLocation(index), text_value, m_dictItem.GetLen());

        // because we already have the text value input, this flag prevents
        // SetValue from converting the double to the string buffer
        m_settingValueFromTextInput = true;
        FixedWidthNumericCaseItem::SetValueFromTextInput(index, text_value);
        m_settingValueFromTextInput = false;
    }

    void OutputFixedValue(const CaseItemIndex& index, TCHAR* text_buffer) const override
    {
        _tmemcpy(text_buffer, GetStringBufferLocation(index), m_dictItem.GetLen());
    }

    /// <summary>
    /// Gets the string representation of the current value.
    /// </summary>
    CString GetStringBufferValue(const CaseItemIndex& index) const
    {
        return CString(GetStringBufferLocation(index), m_dictItem.GetLen());
    }

private:
    const TCHAR* GetStringBufferLocation(const void* data_buffer) const
    {
        return (const TCHAR*)( (std::byte*)data_buffer + sizeof(double) );
    }

    TCHAR* GetStringBufferLocation(void* data_buffer) const
    {
        return (TCHAR*)(GetStringBufferLocation((const void*)data_buffer));
    }

    const TCHAR* GetStringBufferLocation(const CaseItemIndex& index) const
    {
        return GetStringBufferLocation(GetDataBuffer(index));
    }

    TCHAR* GetStringBufferLocation(CaseItemIndex& index) const
    {
        return const_cast<TCHAR*>(GetStringBufferLocation(std::as_const(index)));
    }

    // CR_TODO the use of the flag m_settingValueFromTextInput is not thread safe
    mutable bool m_settingValueFromTextInput;
};
