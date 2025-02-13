#pragma once

#include <zCaseO/zCaseO.h>
#include <zCaseO/CaseItem.h>


class ZCASEO_API StringCaseItem : public CaseItem
{
    friend class CaseItem;

protected:
    StringCaseItem(const CDictItem& dict_item, Type type = Type::String);

    size_t GetSizeForMemoryAllocation() const override;
    void AllocateMemory(void* data_buffer) const override;
    void DeallocateMemory(void* data_buffer) const override;

    void ResetValue(void* data_buffer) const override
    {
        CString& string = *static_cast<CString*>(data_buffer);
        string.Empty();
    }

    void CopyValue(void* data_buffer, const void* copy_data_buffer) const override
    {
        CString& string = *static_cast<CString*>(data_buffer);
        const CString& copy_string = *static_cast<const CString*>(copy_data_buffer);
        string = copy_string;
    }

    size_t StoreBinaryValue(const void* data_buffer, std::byte* binary_buffer) const override;
    size_t RetrieveBinaryValue(void* data_buffer, const std::byte* binary_buffer) const override;

public:
    bool IsBlank(const CaseItemIndex& index) const override
    {
        return GetValue(index).IsEmpty();
    }

    int CompareValues(const CaseItemIndex& index1, const CaseItemIndex& index2) const override;

    /// <summary>
    /// Gets the string case item.
    /// </summary>
    const CString& GetValue(const CaseItemIndex& index) const
    {
        return *static_cast<const CString*>(GetDataBuffer(index));
    }

    /// <summary>
    /// Sets the string case item.
    /// </summary>
    virtual void SetValue(CaseItemIndex& index, const CString& value) const 
    {
        *static_cast<CString*>(GetDataBuffer(index)) = value;
        RunPostSetValueTasks(index);
    }

    /// <summary>
    /// Gets the string case item as a string view.
    /// </summary>
    wstring_view GetValueSV(const CaseItemIndex& index) const
    {
        return GetValue(index);
    }
};
