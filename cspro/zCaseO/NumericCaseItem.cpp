#include "stdafx.h"
#include "NumericCaseItem.h"


NumericCaseItem::NumericCaseItem(const CDictItem& dict_item, Type type/* = Type::Numeric*/)
    :   CaseItem(dict_item, type)
{
    if( !dict_item.HasValueSets() )
        return;

    // if there is a value set, determine if the special values are mapped to anything
    for( const DictValue& dict_value : dict_item.GetValueSet(0).GetValues() )
    {
        if( dict_value.IsSpecial() && dict_value.HasValuePairs() )
        {
            const auto& dict_value_pair = dict_value.GetValuePair(0);
            std::wstring trimmed_from_value = SO::Trim(dict_value_pair.GetFrom());

            if( CIMSAString::IsNumeric(wstring_view(trimmed_from_value), false) )
            {
                double value = atod(trimmed_from_value);
                double special_value = dict_value.GetSpecialValue();

                if( !m_specialToSerializedValues.has_value() )
                {
                    m_specialToSerializedValues = VectorMap<double, double>();
                    m_serializedToSpecialValues = VectorMap<double, double>();
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


size_t NumericCaseItem::StoreBinaryValue(const void* data_buffer, std::byte* binary_buffer) const
{
    if( binary_buffer != nullptr )
        memcpy(binary_buffer, data_buffer, sizeof(double));

    return sizeof(double);
}


size_t NumericCaseItem::RetrieveBinaryValue(void* data_buffer, const std::byte* binary_buffer) const
{
    memcpy(data_buffer, binary_buffer, sizeof(double));
    return sizeof(double);
}


double NumericCaseItem::GetValueForComparison(const CaseItemIndex& index) const
{
    double value = GetValueForOutput(index);
    return ( value == NOTAPPL ) ? std::numeric_limits<double>::lowest() : value;
}


int NumericCaseItem::CompareValues(const CaseItemIndex& index1, const CaseItemIndex& index2) const
{
    double value1 = GetValueForComparison(index1);
    double value2 = GetValueForComparison(index2);

    return ( value1 == value2 ) ?  0 :
           ( value1 < value2 )  ? -1 :
                                   1;
}
