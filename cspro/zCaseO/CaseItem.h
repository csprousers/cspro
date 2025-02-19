#pragma once

#include <zCaseO/zCaseO.h>
#include <zCaseO/CaseItemIndex.h>
#include <zCaseO/IPostSetValueTask.h>
#include <zUtilO/DataTypes.h>
#include <zDictO/ItemIndexHelper.h>


// --------------------------------------------------------------------------
// CaseItem
// --------------------------------------------------------------------------

class ZCASEO_API CaseItem
{
    friend class CaseItemIndex;
    friend class CaseRecord;
    friend class CaseRecordMetadata;

public:
    enum class Type
    {
        String,
        FixedWidthString,
        Numeric,
        FixedWidthNumeric,
        FixedWidthNumericWithStringBuffer,
        Binary
    };

protected:
    // Case items must be created using the static Create method.
    CaseItem(const CDictItem& dict_item, Type type, DataType data_type, bool fixed_width);

public:
    CaseItem(const CaseItem&) = delete;
    virtual ~CaseItem() { }

protected:
    // Gets the number of bytes needed to store this item.
    virtual size_t GetSizeForMemoryAllocation() const = 0;

    // Allocates memory for this item. This method does not need to set
    // an initial value for the item as this will happen in ResetValue.
    virtual void AllocateMemory(void* data_buffer) const = 0;

    // Deallocates memory for this item.
    virtual void DeallocateMemory(void* data_buffer) const = 0;

    // Resets the value to its default value.
    virtual void ResetValue(void* data_buffer) const = 0;

    // Copys a value from another case item of the same type. This is only used
    // when copying a whole case so no IPostSetValueTask tasks are run.
    virtual void CopyValue(void* data_buffer, const void* copy_data_buffer) const = 0;

    // Stores the value in the buffer in binary form and returns the number of bytes
    // used to store the value. If binary_buffer is null, then only the number of bytes
    // needed to store the value is returned.
    virtual size_t StoreBinaryValue(const void* data_buffer, std::byte* binary_buffer) const = 0;

    // Retrieves the value from the binary buffer, advancing the buffer past the read bytes.
    virtual void RetrieveBinaryValue(void* data_buffer, const std::byte*& binary_buffer) const = 0;

    // Converts the index to the data buffer value used to access the case item's memory.
    const void* GetDataBuffer(const CaseItemIndex& index) const;
    void* GetDataBuffer(CaseItemIndex& index) const { return const_cast<void*>(GetDataBuffer(std::as_const(index))); }

    // Runs tasks following the setting of a value.
    void RunPostSetValueTasks(CaseItemIndex& index) const;

    // Adds a task to be run following the setting of a value.
    void AddPostSetValueTask(std::shared_ptr<IPostSetValueTask> post_set_value_task) { m_postSetValueTasks.emplace_back(std::move(post_set_value_task)); }

public:
    // Constructs a case item based on the dictionary item.
    static std::unique_ptr<CaseItem> Create(const CDictItem& dict_item);

    // Gets the dictionary item associated with the case item.
    const CDictItem& GetDictItem() const { return m_dictItem; }

    // Gets the item index helper associated with the case item.
    const ItemIndexHelper& GetItemIndexHelper() const { return m_itemIndexHelper; }

    // Gets the type of the case item.
    Type GetType() const { return m_type; }

    // Gets the DataType of the case item.
    DataType GetDataType() const { return m_dataType; }

    // Returns whether the type is one of the fixed width types.
    bool IsFixedWidth() const { return m_fixedWidth; }

    // Gets the total number of occurrences (item or subitem) for the item.
    size_t GetTotalNumberItemSubitemOccurrences() const { return m_totalNumberOccurrences; }

    // Gets whether or not the value at the given index is blank (or not set, whatever that means for the case item).
    virtual bool IsBlank(const CaseItemIndex& index) const = 0;

    // Compares the values stored at the two indices.
    virtual int CompareValues(const CaseItemIndex& index1, const CaseItemIndex& index2) const = 0;

private:
    static std::unique_ptr<CaseItem> Create(const CDictItem& dict_item, Type type);

    void RunPostSetValueTask(CaseItemIndex& index, const std::vector<std::shared_ptr<IPostSetValueTask>>::const_iterator& task_iterator) const;

protected:
    const CDictItem& m_dictItem;
    const ItemIndexHelper m_itemIndexHelper;

    const Type m_type;
    const DataType m_dataType;
    const bool m_fixedWidth;

    // data access and occurrence variables
    size_t m_recordDataOffset;
    size_t m_memorySize;
    size_t m_totalCaseItemIndex;

    size_t m_totalNumberOccurrences;
    size_t m_itemOccurrenceMultiplier;
    const CDictItem* m_parentDictionaryItem;
    bool m_hasMultipleOccurrences;

private:
    std::vector<std::shared_ptr<IPostSetValueTask>> m_postSetValueTasks;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline void CaseItem::RunPostSetValueTasks(CaseItemIndex& index) const
{
    if( !m_postSetValueTasks.empty() )
        RunPostSetValueTask(index, m_postSetValueTasks.cbegin());
}
