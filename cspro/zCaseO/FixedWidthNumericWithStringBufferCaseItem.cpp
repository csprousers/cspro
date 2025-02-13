#include "stdafx.h"
#include "FixedWidthNumericWithStringBufferCaseItem.h"


FixedWidthNumericWithStringBufferCaseItem::FixedWidthNumericWithStringBufferCaseItem(const CDictItem& dict_item)
    :   FixedWidthNumericCaseItem(dict_item, Type::FixedWidthNumericWithStringBuffer),
        m_settingValueFromTextInput(false)
{
}


size_t FixedWidthNumericWithStringBufferCaseItem::GetSizeForMemoryAllocation() const
{
    // space for the double and then for the string buffer
    return sizeof(double) + ( sizeof(TCHAR) * m_dictItem.GetLen() );
}


void FixedWidthNumericWithStringBufferCaseItem::AllocateMemory(void* /*data_buffer*/) const
{
}


void FixedWidthNumericWithStringBufferCaseItem::DeallocateMemory(void* /*data_buffer*/) const
{
}


size_t FixedWidthNumericWithStringBufferCaseItem::StoreBinaryValue(const void* data_buffer, std::byte* binary_buffer) const
{
    if( binary_buffer != nullptr )
        memcpy(binary_buffer, data_buffer, GetSizeForMemoryAllocation());

    return GetSizeForMemoryAllocation();
}


size_t FixedWidthNumericWithStringBufferCaseItem::RetrieveBinaryValue(void* data_buffer, const std::byte* binary_buffer) const
{
    memcpy(data_buffer, binary_buffer, GetSizeForMemoryAllocation());

    return GetSizeForMemoryAllocation();
}
