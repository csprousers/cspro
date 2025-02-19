#include "stdafx.h"
#include "FixedWidthStringCaseItem.h"


FixedWidthStringCaseItem::FixedWidthStringCaseItem(const CDictItem& dict_item)
    :   StringCaseItem(dict_item, Type::FixedWidthString, true)
{
}


void FixedWidthStringCaseItem::AllocateMemory(void* data_buffer) const
{
    StringCaseItem::AllocateMemory(data_buffer);

    // set the length of the string
    SharableString& value = GetSharableString(data_buffer);
    FixedWidthStringCaseItem::AdjustValueForType(value);
}


void FixedWidthStringCaseItem::ResetValue(void* data_buffer) const
{
    std::string& value = GetSharableString(data_buffer).MakeModifiable();
    ASSERT(SO::WideLength(value) == m_dictItem.GetLen());

    memset(value.data(), ' ', m_dictItem.GetLen());
    value.resize(m_dictItem.GetLen());
}


bool FixedWidthStringCaseItem::IsBlank(const CaseItemIndex& index) const
{
    const std::string& value = GetString(index);
    return SO::IsBlank(value);
}


void FixedWidthStringCaseItem::SetFixedWidthValue(CaseItemIndex& index, const std::string_view value_sv) const
{
    ASSERT(SO::WideLength(value_sv) == m_dictItem.GetLen());
    SetValue(index, value_sv);
}


void FixedWidthStringCaseItem::SetFixedWidthValue(CaseItemIndex& index, const char* const value) const
{
    SetFixedWidthValue(index, std::string_view(value, SO::WideGetOffset(value, m_dictItem.GetLen())));
}


void FixedWidthStringCaseItem::SetFixedWidthValue(CaseItemIndex& index, const TCHAR* value) const
{
    SetValue(index, UTF8_TODO::GetUtf8(std::wstring_view(value, m_dictItem.GetLen())));
}


void FixedWidthStringCaseItem::AdjustValueForType(SharableString& value) const
{
    value.WideMakeExactLength(m_dictItem.GetLen());
}


size_t FixedWidthStringCaseItem::GetMaxUtf8FixedValueWidth() const
{
    return m_dictItem.GetLen() * TC::MaxUtf8BytesNeededForWideChar();
}


size_t FixedWidthStringCaseItem::OutputFixedValue(const CaseItemIndex& index, char* const text_buffer) const
{
    const std::string& value = GetString(index);
    memcpy(text_buffer, value.data(), value.length());
    return value.length();
}
