#include "stdafx.h"
#include "StringCaseItem.h"


StringCaseItem::StringCaseItem(const CDictItem& dict_item, Type type/* = Type::String*/)
    :   CaseItem(dict_item, type)
{
}


size_t StringCaseItem::GetSizeForMemoryAllocation() const
{
    return sizeof(CString);
}


void StringCaseItem::AllocateMemory(void* data_buffer) const
{
    new(data_buffer) CString();
}


void StringCaseItem::DeallocateMemory(void* data_buffer) const 
{
    static_cast<CString*>(data_buffer)->~CString();
}


size_t StringCaseItem::StoreBinaryValue(const void* data_buffer, std::byte* binary_buffer) const
{
    const CString& string = *((const CString*)data_buffer);

    if( binary_buffer != nullptr )
    {
        *reinterpret_cast<int*>(binary_buffer) = (int)string.GetLength();
        binary_buffer += sizeof(int);
        memcpy(binary_buffer, (LPCTSTR)string, sizeof(TCHAR) * string.GetLength());
    }

    return sizeof(int) + ( sizeof(TCHAR) * string.GetLength() );
}


size_t StringCaseItem::RetrieveBinaryValue(void* data_buffer, const std::byte* binary_buffer) const 
{
    CString& string = *static_cast<CString*>(data_buffer);
    int string_length = *reinterpret_cast<const int*>(binary_buffer);
    binary_buffer += sizeof(int);

    TCHAR* string_buffer = string.GetBufferSetLength(string_length);
    memcpy(string_buffer, binary_buffer, sizeof(TCHAR) * string_length);
    string.ReleaseBuffer(string_length);

    return sizeof(int) + ( sizeof(TCHAR) * string.GetLength() );
}


int StringCaseItem::CompareValues(const CaseItemIndex& index1, const CaseItemIndex& index2) const
{
    return GetValue(index1).Compare(GetValue(index2));
}
