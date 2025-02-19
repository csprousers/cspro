#pragma once

#include <zCaseO/zCaseO.h>
#include <zCaseO/CaseItem.h>
#include <zUtilO/VectorMap.h>


class ZCASEO_API NumericCaseItem : public CaseItem
{
    friend class CaseItem;

protected:
    NumericCaseItem(const CDictItem& dict_item, Type type = Type::Numeric, bool fixed_width = false);

    size_t GetSizeForMemoryAllocation() const override;
    void AllocateMemory(void* data_buffer) const override;
    void DeallocateMemory(void* data_buffer) const override;

    void ResetValue(void* data_buffer) const override;
    void CopyValue(void* data_buffer, const void* copy_data_buffer) const override;

    size_t StoreBinaryValue(const void* data_buffer, std::byte* binary_buffer) const override;
    void RetrieveBinaryValue(void* data_buffer, const std::byte*& binary_buffer) const override;

public:
    // Returns true if the string if not NOTAPPL.
    bool IsBlank(const CaseItemIndex& index) const override;

    int CompareValues(const CaseItemIndex& index1, const CaseItemIndex& index2) const override;

    // Gets the numeric value.
    double GetValue(const CaseItemIndex& index) const { return GetValue(GetDataBuffer(index)); }

    // Gets the numeric value for saving to an output source.
    // Missing and refused values will be converted to the appropriate value for serializing.
    double GetValueForOutput(const CaseItemIndex& index) const { return GetValueForOutput(GetValue(index)); }
    double GetValueForOutput(double value) const;

    // Gets the numeric value that can be used for comparisons (with NOTAPPL values being sorted before all other numbers).
    double GetValueForComparison(const CaseItemIndex& index) const;

    // Sets the numeric value.
    virtual void SetValue(CaseItemIndex& index, double value) const;

    // Sets the numeric value to NOTAPPL.
    void SetNotappl(CaseItemIndex& index) const;

    // Sets the numeric value from a value retrieved from an input source.
    // Special values will be converted to the appropriate value.
    void SetValueFromInput(CaseItemIndex& index, double value) const;

private:
    static double GetValue(const void* data_buffer)        { return *static_cast<const double*>(data_buffer); }
    static double& GetModifiableValue(void* data_buffer)   { return *static_cast<double*>(data_buffer); }
    double& GetModifiableValue(CaseItemIndex& index) const { return GetModifiableValue(GetDataBuffer(index)); }

    static void AdjustValueForSpecialCoding(const VectorMap<double, double>& values_map, double& value);

private:
    std::unique_ptr<VectorMap<double, double>> m_specialToSerializedValues;
    std::unique_ptr<VectorMap<double, double>> m_serializedToSpecialValues;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline double NumericCaseItem::GetValueForOutput(double value) const
{
    if( m_specialToSerializedValues != nullptr )
        AdjustValueForSpecialCoding(*m_specialToSerializedValues, value);

    return value;
}


inline void NumericCaseItem::SetValue(CaseItemIndex& index, const double value) const
{
    double& this_value = GetModifiableValue(index);
    this_value = value;

    RunPostSetValueTasks(index);
}


inline void NumericCaseItem::SetValueFromInput(CaseItemIndex& index, double value) const
{
    if( m_serializedToSpecialValues != nullptr )
        AdjustValueForSpecialCoding(*m_serializedToSpecialValues, value);

    SetValue(index, value);
}
