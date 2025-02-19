#include "stdafx.h"
#include "StringCaseItem.h"


StringCaseItem::StringCaseItem(const CDictItem& dict_item, const Type type/* = Type::String*/, const bool fixed_width/* = false*/)
    :   CaseItem(dict_item, type, DataType::String, fixed_width)
{
}


size_t StringCaseItem::GetSizeForMemoryAllocation() const
{
    return sizeof(SharableString);
}


void StringCaseItem::AllocateMemory(void* const data_buffer) const
{
    new(data_buffer) SharableString();
}


void StringCaseItem::DeallocateMemory(void* const data_buffer) const
{
    SharableString& value = GetSharableString(data_buffer);
    value.~SharableString();
}


void StringCaseItem::ResetValue(void* const data_buffer) const
{
    SharableString& value = GetSharableString(data_buffer);
    value.Reset();
}


void StringCaseItem::CopyValue(void* const data_buffer, const void* const copy_data_buffer) const
{
    SharableString& value = GetSharableString(data_buffer);
    const SharableString& copy_value = GetSharableString(copy_data_buffer);
    value = copy_value;
}


size_t StringCaseItem::StoreBinaryValue(const void* data_buffer, std::byte* binary_buffer) const
{
    const SharableString& value = GetSharableString(data_buffer);
    return BinarySerializer::Write(binary_buffer, *value);
}


void StringCaseItem::RetrieveBinaryValue(void* const data_buffer, const std::byte*& binary_buffer) const
{
    SharableString& value = GetSharableString(data_buffer);
    BinarySerializer::Read(binary_buffer, value.MakeModifiable());
}


bool StringCaseItem::IsBlank(const CaseItemIndex& index) const
{
    const std::string& value = GetValue(index);
    return value.empty();
}


int StringCaseItem::CompareValues(const CaseItemIndex& index1, const CaseItemIndex& index2) const
{
    const std::string& value1 = GetValue(index1);
    const std::string& value2 = GetValue(index2);
    return value1.compare(value2);
}


void StringCaseItem::AdjustValueForType(SharableString& /*value*/) const
{
}
