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
    size_t RetrieveBinaryValue(void* data_buffer, const std::byte* binary_buffer) const override;

public:
    bool IsBlank(const CaseItemIndex& index) const override;

    int CompareValues(const CaseItemIndex& index1, const CaseItemIndex& index2) const override;

    // returns a BinaryDataAccessor object that can be used to access binary data;
    // exceptions can be thrown when using some of the object's methods if a binary data reader is being used
    const BinaryDataAccessor& GetBinaryDataAccessor(const CaseItemIndex& index) const;

    // returns a BinaryDataAccessor object that can be used to set binary data
    BinaryDataAccessor& GetBinaryDataAccessor(CaseItemIndex& index) const;

    // these are wrappers around BinaryDataAccessor methods that catch any exceptions thrown by a binary data reader;
    // the exception is logged (if using a case construction reporter) and then a null or std::nullopt value is returned;
    // if binary data is not defined, the methods return null or std::nullopt
    const BinaryData* GetBinaryData_noexcept(const CaseItemIndex& index) const noexcept;
    const BinaryDataMetadata* GetBinaryDataMetadata_noexcept(const CaseItemIndex& index) const noexcept;
    std::optional<uint64_t> GetBinaryDataSize_noexcept(const CaseItemIndex& index) const noexcept;

private:
    static void HandleException(const CSProException& exception, const CaseItemIndex& index);

    const BinaryData* GetBinaryData_noexcept(const CaseItemIndex& index, const BinaryDataAccessor& binary_data_accessor) const noexcept;
    const BinaryDataMetadata* GetBinaryDataMetadata_noexcept(const CaseItemIndex& index, const BinaryDataAccessor& binary_data_accessor) const noexcept;
    std::optional<uint64_t> GetBinaryDataSize_noexcept(const CaseItemIndex& index, const BinaryDataAccessor& binary_data_accessor) const noexcept;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline void BinaryCaseItem::ResetValue(void* data_buffer) const
{
    static_cast<BinaryDataAccessor*>(data_buffer)->Reset();
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


inline const BinaryData* BinaryCaseItem::GetBinaryData_noexcept(const CaseItemIndex& index) const noexcept
{
    return GetBinaryData_noexcept(index, GetBinaryDataAccessor(index));
}


inline const BinaryDataMetadata* BinaryCaseItem::GetBinaryDataMetadata_noexcept(const CaseItemIndex& index) const noexcept
{
    return GetBinaryDataMetadata_noexcept(index, GetBinaryDataAccessor(index));
}


inline std::optional<uint64_t> BinaryCaseItem::GetBinaryDataSize_noexcept(const CaseItemIndex& index) const noexcept
{
    return GetBinaryDataSize_noexcept(index, GetBinaryDataAccessor(index));
}
