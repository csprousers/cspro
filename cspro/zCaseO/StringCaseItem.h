#pragma once

#include <zCaseO/zCaseO.h>
#include <zCaseO/CaseItem.h>


class ZCASEO_API StringCaseItem : public CaseItem
{
    friend class CaseItem;

protected:
    StringCaseItem(const CDictItem& dict_item, Type type = Type::String, bool fixed_width = false);

    size_t GetSizeForMemoryAllocation() const override;
    void AllocateMemory(void* data_buffer) const override;
    void DeallocateMemory(void* data_buffer) const override;

    void ResetValue(void* data_buffer) const override;
    void CopyValue(void* data_buffer, const void* copy_data_buffer) const override;

    size_t StoreBinaryValue(const void* data_buffer, std::byte* binary_buffer) const override;
    void RetrieveBinaryValue(void* data_buffer, const std::byte*& binary_buffer) const override;

public:
    // Returns true if the string if non-empty.
    bool IsBlank(const CaseItemIndex& index) const override;

    int CompareValues(const CaseItemIndex& index1, const CaseItemIndex& index2) const override;

    // Gets the string value.
    const std::string& GetValue(const CaseItemIndex& index) const             { return *GetSharableString(index); }
    const std::string& GetString(const CaseItemIndex& index) const            { return GetValue(index); }
    const SharableString& GetSharableString(const CaseItemIndex& index) const { return GetSharableString(GetDataBuffer(index)); }

    // Sets the string value.
    template<typename T>
    void SetValue(CaseItemIndex& index, T&& value) const;

    template<typename T>
    void SetString(CaseItemIndex& index, T&& value) const { return SetValue(GetSharableString(index), std::forward<T>(value)); }

protected:
    virtual void AdjustValueForType(SharableString& value) const;

    static const SharableString& GetSharableString(const void* data_buffer) { return *static_cast<const SharableString*>(data_buffer); }
    static SharableString& GetSharableString(void* data_buffer)             { return *static_cast<SharableString*>(data_buffer); }
    SharableString& GetSharableString(CaseItemIndex& index) const           { return GetSharableString(GetDataBuffer(index)); }

    template<typename T>
    void SetStringWithoutRunningPostSetValueTasks(void* data_buffer, T&& value) const;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

template<typename T>
void StringCaseItem::SetValue(CaseItemIndex& index, T&& value) const
{
    SetStringWithoutRunningPostSetValueTasks(GetDataBuffer(index), std::forward<T>(value));

    RunPostSetValueTasks(index);
}


template<typename T>
void StringCaseItem::SetStringWithoutRunningPostSetValueTasks(void* const data_buffer, T&& value) const
{
    SharableString& this_value = GetSharableString(data_buffer);
    this_value = std::forward<T>(value);

    AdjustValueForType(this_value);
}
