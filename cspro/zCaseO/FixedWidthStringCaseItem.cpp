#include "stdafx.h"
#include "FixedWidthStringCaseItem.h"


FixedWidthStringCaseItem::FixedWidthStringCaseItem(const CDictItem& dict_item)
    :   StringCaseItem(dict_item, Type::FixedWidthString)
{
}


void FixedWidthStringCaseItem::AllocateMemory(void* data_buffer) const
{
    StringCaseItem::AllocateMemory(data_buffer);

    // set the length of the string
    CString& string = *static_cast<CString*>(data_buffer);
    string.GetBufferSetLength(m_dictItem.GetLen());
    string.ReleaseBuffer(m_dictItem.GetLen());
}
