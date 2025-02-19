#include "stdafx.h"
#include "FixedWidthNumericWithStringBufferCaseItem.h"
#include <zToolsO/RaiiHelpers.h>


namespace
{
    constexpr size_t OffsetToStringBuffer = sizeof(double);

    const void* GetOffsetToStringBuffer(const void* const data_buffer) { return static_cast<const std::byte*>(data_buffer) + OffsetToStringBuffer; }
    void* GetOffsetToStringBuffer(void* const data_buffer)             { return static_cast<std::byte*>(data_buffer) + OffsetToStringBuffer; }
}


FixedWidthNumericWithStringBufferCaseItem::FixedWidthNumericWithStringBufferCaseItem(const CDictItem& dict_item)
    :   FixedWidthNumericCaseItem(dict_item, Type::FixedWidthNumericWithStringBuffer),
        m_bufferStringCaseItem(dict_item),
        m_settingValueFromTextInput(false)
{
}


size_t FixedWidthNumericWithStringBufferCaseItem::GetSizeForMemoryAllocation() const
{
    // space for the double and then for the string buffer
    ASSERT(FixedWidthNumericCaseItem::GetSizeForMemoryAllocation() == OffsetToStringBuffer);

    return FixedWidthNumericCaseItem::GetSizeForMemoryAllocation() + m_bufferStringCaseItem.GetSizeForMemoryAllocation();
}


void FixedWidthNumericWithStringBufferCaseItem::AllocateMemory(void* const data_buffer) const
{
    FixedWidthNumericCaseItem::AllocateMemory(data_buffer);
    m_bufferStringCaseItem.AllocateMemory(GetOffsetToStringBuffer(data_buffer));
}


void FixedWidthNumericWithStringBufferCaseItem::DeallocateMemory(void* const data_buffer) const
{
    m_bufferStringCaseItem.DeallocateMemory(GetOffsetToStringBuffer(data_buffer));
    FixedWidthNumericCaseItem::DeallocateMemory(data_buffer);
}


void FixedWidthNumericWithStringBufferCaseItem::ResetValue(void* const data_buffer) const
{
    m_bufferStringCaseItem.ResetValue(GetOffsetToStringBuffer(data_buffer));
    FixedWidthNumericCaseItem::ResetValue(data_buffer);
}


void FixedWidthNumericWithStringBufferCaseItem::CopyValue(void* const data_buffer, const void* const copy_data_buffer) const
{
    m_bufferStringCaseItem.CopyValue(GetOffsetToStringBuffer(data_buffer), GetOffsetToStringBuffer(copy_data_buffer));
    FixedWidthNumericCaseItem::CopyValue(data_buffer, copy_data_buffer);
}


size_t FixedWidthNumericWithStringBufferCaseItem::StoreBinaryValue(const void* const data_buffer, std::byte* binary_buffer) const
{
    const size_t binary_buffer_size = FixedWidthNumericCaseItem::StoreBinaryValue(data_buffer, binary_buffer);

    if( binary_buffer != nullptr )
        binary_buffer += binary_buffer_size;

    return binary_buffer_size + m_bufferStringCaseItem.StoreBinaryValue(GetOffsetToStringBuffer(data_buffer), binary_buffer);
}


void FixedWidthNumericWithStringBufferCaseItem::RetrieveBinaryValue(void* const data_buffer, const std::byte*& binary_buffer) const
{
    FixedWidthNumericCaseItem::RetrieveBinaryValue(data_buffer, binary_buffer);

    m_bufferStringCaseItem.RetrieveBinaryValue(GetOffsetToStringBuffer(data_buffer), binary_buffer);
}


void FixedWidthNumericWithStringBufferCaseItem::SetValue(CaseItemIndex& index, const double value) const
{
    if( !m_settingValueFromTextInput )
    {
        auto wide_buffer = std::make_unique_for_overwrite<wchar_t[]>(m_dictItem.GetLen());
        FixedWidthNumericCaseItem::ConvertNumberToText(GetValueForOutput(value), wide_buffer.get());

        void* const data_buffer = GetDataBuffer(index);
        m_bufferStringCaseItem.SetStringWithoutRunningPostSetValueTasks(GetOffsetToStringBuffer(data_buffer), UTF8_TODO::GetUtf8(std::wstring_view(wide_buffer.get(), m_dictItem.GetLen())));
    }

    FixedWidthNumericCaseItem::SetValue(index, value);
}


void FixedWidthNumericWithStringBufferCaseItem::SetValueFromTextInput(CaseItemIndex& index, const TCHAR* const text_value) const
{
    void* const data_buffer = GetDataBuffer(index);
    m_bufferStringCaseItem.SetStringWithoutRunningPostSetValueTasks(GetOffsetToStringBuffer(data_buffer), UTF8_TODO::GetUtf8(std::wstring_view(text_value, m_dictItem.GetLen())));

    // because we already have the text value input, this flag prevents
    // SetValue from converting the double to the string buffer
    const RAII::SetValueAndRestoreOnDestruction rod(m_settingValueFromTextInput, true);

    FixedWidthNumericCaseItem::SetValueFromTextInput(index, text_value);
}


size_t FixedWidthNumericWithStringBufferCaseItem::OutputFixedValue(const CaseItemIndex& index, char* const text_buffer) const
{
    const void* const data_buffer = GetDataBuffer(index);
    const SharableString& value = m_bufferStringCaseItem.GetSharableString(GetOffsetToStringBuffer(data_buffer));

    ASSERT(value->length() <= GetMaxUtf8FixedValueWidth());

    memcpy(text_buffer, value->data(), value->length());

    return value->length();
}


const SharableString& FixedWidthNumericWithStringBufferCaseItem::GetBufferSharableString(const CaseItemIndex& index) const
{
    const void* const data_buffer = GetDataBuffer(index);
    return m_bufferStringCaseItem.GetSharableString(GetOffsetToStringBuffer(data_buffer));
}
