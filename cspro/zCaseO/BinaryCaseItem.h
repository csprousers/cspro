#pragma once

#include <zCaseO/zCaseO.h>
#include <zCaseO/CaseItem.h>
#include <zUtilO/BinaryDataAccessor.h>


class ZCASEO_API BinaryCaseItem : public CaseItem
{
    friend class CaseItem;

protected:
    BinaryCaseItem(const CDictItem& dict_item);

    size_t GetSizeForMemoryAllocation() const override;
    void AllocateMemory(void* data_buffer) const override;
    void DeallocateMemory(void* data_buffer) const override;

    void ResetValue(void* data_buffer) const override;
    void CopyValue(void* data_buffer, const void* copy_data_buffer) const override;

    size_t StoreBinaryValue(const void* data_buffer, std::byte* binary_buffer) const override;
    void RetrieveBinaryValue(void* data_buffer, const std::byte*& binary_buffer) const override;

public:
    bool IsBlank(const CaseItemIndex& index) const override;

    int CompareValues(const CaseItemIndex& index1, const CaseItemIndex& index2) const override;

    // Returns a BinaryDataAccessor object that can be used to access binary data.
    // Exceptions can be thrown when using some of the object's methods if a binary content reader is being used.
    const BinaryDataAccessor& GetBinaryDataAccessor(const CaseItemIndex& index) const;
    BinaryDataAccessor& GetBinaryDataAccessor(CaseItemIndex& index) const;

    // Modifies the item's value by assigning a BinaryDataAccessor constructed using the templated arguments.
    template<typename... Args>
    void SetValue(CaseItemIndex& index, Args&&... args) const;

    // Clears any binary data held by the item.
    void Clear(CaseItemIndex& index) const;

    // These are wrappers around BinaryDataAccessor methods that catch any exceptions thrown by a binary content reader.
    // The exception is logged (if using a case construction reporter) and then a null or std::nullopt value is returned.
    // If binary data is not defined, the methods return null or std::nullopt.
    const BinaryData* GetBinaryData_noexcept(const CaseItemIndex& index) const noexcept;
    std::optional<uint64_t> GetBinaryDataSize_noexcept(const CaseItemIndex& index) const noexcept;

    // Returns a suggested filename for the binary data. If a filename is defined in the metadata, it is returned,
    // otherwise a filename is created based on the dictionary item name and index.
    std::string GetSuggestedFilename(const CaseItemIndex& index) const;

private:
    static void HandleException(const CSProException& exception, const CaseItemIndex& index);

    const BinaryData* GetBinaryData_noexcept(const CaseItemIndex& index, const BinaryDataAccessor& binary_data_accessor) const noexcept;
    std::optional<uint64_t> GetBinaryDataSize_noexcept(const CaseItemIndex& index, const BinaryDataAccessor& binary_data_accessor) const noexcept;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline void BinaryCaseItem::ResetValue(void* const data_buffer) const
{
    static_cast<BinaryDataAccessor*>(data_buffer)->Clear();
}


inline bool BinaryCaseItem::IsBlank(const CaseItemIndex& index) const
{
    return !GetBinaryDataAccessor(index).IsDefined();
}


inline const BinaryDataAccessor& BinaryCaseItem::GetBinaryDataAccessor(const CaseItemIndex& index) const
{
    return *static_cast<const BinaryDataAccessor*>(GetDataBuffer(index));
}


inline BinaryDataAccessor& BinaryCaseItem::GetBinaryDataAccessor(CaseItemIndex& index) const
{
    return *static_cast<BinaryDataAccessor*>(GetDataBuffer(index));
}


template<typename... Args>
void BinaryCaseItem::SetValue(CaseItemIndex& index, Args&&... args) const
{
    *static_cast<BinaryDataAccessor*>(GetDataBuffer(index)) = BinaryDataAccessor(std::forward<Args>(args)...);
}


inline const BinaryData* BinaryCaseItem::GetBinaryData_noexcept(const CaseItemIndex& index) const noexcept
{
    return GetBinaryData_noexcept(index, GetBinaryDataAccessor(index));
}


inline std::optional<uint64_t> BinaryCaseItem::GetBinaryDataSize_noexcept(const CaseItemIndex& index) const noexcept
{
    return GetBinaryDataSize_noexcept(index, GetBinaryDataAccessor(index));
}
