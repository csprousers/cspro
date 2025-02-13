#pragma once

#include <zCaseO/zCaseO.h>
#include <zCaseO/CaseItemIndex.h>
#include <zCaseO/IPostSetValueTask.h>
#include <zDictO/DDClass.h>
#include <zDictO/ItemIndexHelper.h>
#include <utility>


class ZCASEO_API CaseItem
{
    friend class CaseRecordMetadata;
    friend class CaseRecord;
    friend class CaseItemIndex;

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
    /// <summary>
    /// Case items must be created using the static Create method.
    /// </summary>
    CaseItem(const CDictItem& dict_item, Type type);

    CaseItem(const CaseItem&) = delete;

    /// <summary>
    /// Gets the number of bytes needed to store this item.
    /// </summary>
    virtual size_t GetSizeForMemoryAllocation() const = 0;

    /// <summary>
    /// Allocates memory for this item. This method does not need to set
    /// an initial value for the item as this will happen in ResetValue.
    /// </summary>
    virtual void AllocateMemory(void* data_buffer) const = 0;

    /// <summary>
    /// Deallocates memory for this item.
    /// </summary>
    virtual void DeallocateMemory(void* data_buffer) const = 0;


    /// <summary>
    /// Resets the value to its default value.
    /// </summary>
    virtual void ResetValue(void* data_buffer) const = 0;

    /// <summary>
    /// Copys a value from another case item of the same type. This is only used
    /// when copying a whole case so no IPostSetValueTask tasks are run.
    /// </summary>
    virtual void CopyValue(void* data_buffer, const void* copy_data_buffer) const = 0;


    /// <summary>
    /// Stores the value in the buffer in binary form and returns the number of bytes
    /// used to store the value. If binary_buffer is nullptr, then only the number of bytes
    /// needed to store the value is returned.
    /// </summary>
    virtual size_t StoreBinaryValue(const void* data_buffer, std::byte* binary_buffer) const = 0;

    /// <summary>
    /// Retrieves the value from the binary buffer and returns the number of
    /// bytes used to store the value.
    /// </summary>
    virtual size_t RetrieveBinaryValue(void* data_buffer, const std::byte* binary_buffer) const = 0;


    /// <summary>
    /// Converts the index to the data buffer value used to access the case item's memory.
    /// </summary>
    const void* GetDataBuffer(const CaseItemIndex& index) const;

    void* GetDataBuffer(CaseItemIndex& index) const
    {
        return const_cast<void*>(GetDataBuffer(std::as_const(index)));
    }


    /// <summary>
    /// Runs tasks following the setting of a value.
    /// </summary>
    void RunPostSetValueTasks(CaseItemIndex& index) const
    {
        if( !m_postSetValueTasks.empty() )
            RunPostSetValueTask(index, m_postSetValueTasks.cbegin());
    }

    /// <summary>
    /// Add a task to be run following the setting of a value.
    /// </summary>
    void AddPostSetValueTask(const std::shared_ptr<IPostSetValueTask>& post_set_value_task)
    {
        m_postSetValueTasks.emplace_back(post_set_value_task);
    }


public:
    virtual ~CaseItem() { }

    /// <summary>
    /// Constructs a case item based on the dictionary item.
    /// </summary>
    static CaseItem* Create(const CDictItem& dict_item);

    /// <summary>
    /// Gets the dictionary item associated with the case item.
    /// </summary>
    const CDictItem& GetDictionaryItem() const { return m_dictItem; }

    /// <summary>
    /// Gets the item index helper associated with the case item.
    /// </summary>
    const ItemIndexHelper& GetItemIndexHelper() const { return m_itemIndexHelper; }

    /// <summary>
    /// Gets the type of the case item.
    /// </summary>
    Type GetType() const { return m_type; }

    /// <summary>
    /// Gets whether the type is one of String or derived.
    /// </summary>
    bool IsTypeString() const { return m_typeString; }

    /// <summary>
    /// Gets whether the type is one of Numeric or derived.
    /// </summary>
    bool IsTypeNumeric() const { return m_typeNumeric; }

    /// <summary>
    /// Gets whether the type is one of Binary or derived.
    /// </summary>
    bool IsTypeBinary() const { return ( m_type == Type::Binary ); }

    /// <summary>
    /// Gets whether the type is one of the fixed width types.
    /// </summary>
    bool IsTypeFixed() const { return m_typeFixed; }

    /// <summary>
    /// Gets the total number of occurrences (item or subitem) for the item.
    /// </summary>
    size_t GetTotalNumberItemSubitemOccurrences() const { return m_totalNumberOccurrences; }


    /// <summary>
    /// Gets whether or not the value at the given index is blank (or not set, whatever that
    /// means for the case item).
    /// </summary>
    virtual bool IsBlank(const CaseItemIndex& index) const = 0;

    /// <summary>
    /// Compares the values stored at the two indices.
    /// </summary>
    virtual int CompareValues(const CaseItemIndex& index1, const CaseItemIndex& index2) const = 0;

protected:
    const CDictItem& m_dictItem;
    const ItemIndexHelper m_itemIndexHelper;

    Type m_type;
    bool m_typeString;
    bool m_typeNumeric;
    bool m_typeFixed;

    // data access and occurrence variables
    size_t m_recordDataOffset;
    size_t m_memorySize;
    size_t m_totalCaseItemIndex;

    size_t m_totalNumberOccurrences;
    size_t m_itemOccurrenceMultiplier;
    const CDictItem* m_parentDictionaryItem;
    bool m_hasMultipleOccurrences;

private:
    static CaseItem* Create(const CDictItem& dict_item, Type type);

    void RunPostSetValueTask(CaseItemIndex& index, const std::vector<std::shared_ptr<IPostSetValueTask>>::const_iterator& task_iterator) const;

    std::vector<std::shared_ptr<IPostSetValueTask>> m_postSetValueTasks;
};


class FixedWidthCaseItem
{
public:
    /// <summary>
    /// Outputs a fixed type case item at the given index using the dictionary properties.
    /// The buffer must have enough space to store the complete length of the case item.
    /// </summary>
    virtual void OutputFixedValue(const CaseItemIndex& index, TCHAR* text_buffer) const = 0;
};
