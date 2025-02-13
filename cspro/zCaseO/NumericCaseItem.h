#pragma once

#include <zCaseO/zCaseO.h>
#include <zCaseO/CaseItem.h>
#include <zToolsO/Special.h>
#include <zUtilO/VectorMap.h>


class ZCASEO_API NumericCaseItem : public CaseItem
{
    friend class CaseItem;

protected:
    NumericCaseItem(const CDictItem& dict_item, Type type = Type::Numeric);

    size_t GetSizeForMemoryAllocation() const override;
    void AllocateMemory(void* data_buffer) const override;
    void DeallocateMemory(void* data_buffer) const override;

    void ResetValue(void* data_buffer) const override
    {
        *static_cast<double*>(data_buffer) = NOTAPPL;
    }

    void CopyValue(void* data_buffer, const void* copy_data_buffer) const override
    {
        *static_cast<double*>(data_buffer) = *static_cast<const double*>(copy_data_buffer);
    }

    size_t StoreBinaryValue(const void* data_buffer, std::byte* binary_buffer) const override;
    size_t RetrieveBinaryValue(void* data_buffer, const std::byte* binary_buffer) const override;

public:
    bool IsBlank(const CaseItemIndex& index) const override
    {
        return ( GetValue(index) == NOTAPPL );
    }

    int CompareValues(const CaseItemIndex& index1, const CaseItemIndex& index2) const override;

    /// <summary>
    /// Gets the numeric case item.
    /// </summary>
    double GetValue(const CaseItemIndex& index) const
    {
        return *static_cast<const double*>(GetDataBuffer(index));
    }

    /// <summary>
    /// Gets the numeric case item's value for saving to an output source. Missing and refused
    /// values will be converted to the appropriate value for serializing.
    /// </summary>
    double GetValueForOutput(const CaseItemIndex& index) const
    {
        return GetValueForOutput(GetValue(index));
    }

    double GetValueForOutput(double value) const
    {
        AdjustValueForSpecialCoding(m_specialToSerializedValues, value);
        return value;
    }

    /// <summary>
    /// Gets the numeric case item's value that can be used for comparisons (with notappl
    /// values being sorted before all other numbers.
    /// </summary>
    double GetValueForComparison(const CaseItemIndex& index) const;

    /// <summary>
    /// Sets the numeric case item.
    /// </summary>
    virtual void SetValue(CaseItemIndex& index, double value) const
    {
        *static_cast<double*>(GetDataBuffer(index)) = value;
        RunPostSetValueTasks(index);
    }

    /// <summary>
    /// Sets the numeric case item to notappl.
    /// </summary>
    void SetNotappl(CaseItemIndex& index) const
    {
        SetValue(index, NOTAPPL);
    }

    /// <summary>
    /// Sets the numeric case item from a value retrieved from an input source. Special values
    /// will be converted to the appropriate value.
    /// </summary>
    void SetValueFromInput(CaseItemIndex& index, double value) const
    {
        AdjustValueForSpecialCoding(m_serializedToSpecialValues, value);
        SetValue(index, value);
    }

private:
    inline void AdjustValueForSpecialCoding(const std::optional<VectorMap<double, double>>& values_map, double& value) const
    {
        if( values_map.has_value() )
        {
            const double* new_value = values_map->Find(value);

            if( new_value != nullptr )
                value = *new_value;
        }
    }

private:
    std::optional<VectorMap<double, double>> m_specialToSerializedValues;
    std::optional<VectorMap<double, double>> m_serializedToSpecialValues;
};
