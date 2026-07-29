#include "stdafx.h"
#include "NumericCaseItem.h"


NumericCaseItem::NumericCaseItem(const CDictItem& dict_item, Type type/* = Type::Numeric*/, const bool fixed_width/* = false*/)
    :   CaseItem(dict_item, type, DataType::Numeric, fixed_width)
{
    if( !dict_item.HasValueSets() )
        return;

    // if there is a value set, determine if the special values are mapped to anything
    for( const DictValue& dict_value : dict_item.GetValueSet(0).GetValues() )
    {
        if( dict_value.IsSpecial() && dict_value.HasValuePairs() )
        {
            const DictValuePair& dict_value_pair = dict_value.GetValuePair(0);
            const std::string trimmed_from_value(SO::Trim(dict_value_pair.GetFrom()));

            if( CIMSAString::IsNumeric(trimmed_from_value, false) )
            {
                const double value = atod(trimmed_from_value);
                const double special_value = dict_value.GetSpecialValue();

                if( m_specialToSerializedValues == nullptr )
                {
                    ASSERT(m_serializedToSpecialValues == nullptr);
                    m_specialToSerializedValues = std::make_unique<VectorMap<double, double>>();
                    m_serializedToSpecialValues = std::make_unique<VectorMap<double, double>>();
                }

                m_specialToSerializedValues->Insert(special_value, value);
                m_serializedToSpecialValues->Insert(value, special_value);
            }
        }
    }
}


size_t NumericCaseItem::GetSizeForMemoryAllocation() const
{
    return sizeof(double);
}


void NumericCaseItem::AllocateMemory(void* /*data_buffer*/) const
{
}


void NumericCaseItem::DeallocateMemory(void* /*data_buffer*/) const
{
}


void NumericCaseItem::ResetValue(void* const data_buffer) const
{
    double& value = GetModifiableValue(data_buffer);
    value = NOTAPPL;
}


void NumericCaseItem::CopyValue(void* const data_buffer, const void* const copy_data_buffer) const
{
    double& value = GetModifiableValue(data_buffer);
    value = GetValue(copy_data_buffer);
}


size_t NumericCaseItem::StoreBinaryValue(const void* const data_buffer, std::byte* const binary_buffer) const
{
    return BinarySerializer::Write(binary_buffer, GetValue(data_buffer));
}


void NumericCaseItem::RetrieveBinaryValue(void* const data_buffer, const std::byte*& binary_buffer) const
{
    double& value = GetModifiableValue(data_buffer);
    BinarySerializer::Read(binary_buffer, value);
}


bool NumericCaseItem::IsBlank(const CaseItemIndex& index) const
{
    return ( GetValue(index) == NOTAPPL );
}


int NumericCaseItem::CompareValues(const CaseItemIndex& index1, const CaseItemIndex& index2) const
{
    const double value1 = GetValueForComparison(index1);
    const double value2 = GetValueForComparison(index2);

    return ( value1 == value2 ) ?  0 :
           ( value1 < value2 )  ? -1 :
                                   1;
}


double NumericCaseItem::GetValueForComparison(const CaseItemIndex& index) const
{
    double value = GetValueForOutput(index);

    if( value == NOTAPPL )
        return std::numeric_limits<double>::lowest();

    return value;
}


void NumericCaseItem::SetNotappl(CaseItemIndex& index) const
{
    SetValue(index, NOTAPPL);
}


void NumericCaseItem::AdjustValueForSpecialCoding(const VectorMap<double, double>& values_map, double& value)
{
    const double* const new_value = values_map.Find(value);

    if( new_value != nullptr )
        value = *new_value;
}
