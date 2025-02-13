#pragma once

#include <zCaseO/zCaseO.h>
#include <zCaseO/StringCaseItem.h>


class ZCASEO_API FixedWidthStringCaseItem : public StringCaseItem, public FixedWidthCaseItem
{
    friend class CaseItem;

protected:
    FixedWidthStringCaseItem(const CDictItem& dict_item);

    void AllocateMemory(void* data_buffer) const override;

    void ResetValue(void* data_buffer) const override
    {
        CString& string = *static_cast<CString*>(data_buffer);
        ASSERT(string.GetLength() == (int)m_dictItem.GetLen());

        _tmemset(string.GetBuffer(), _T(' '), m_dictItem.GetLen());
        string.ReleaseBuffer(m_dictItem.GetLen());
    }

public:
    bool IsBlank(const CaseItemIndex& index) const override
    {
        const CString& string = *static_cast<const CString*>(GetDataBuffer(index));
        return SO::IsBlank(string);
    }

    void SetValue(CaseItemIndex& index, const CString& value) const override
    {
        CString& string = *static_cast<CString*>(GetDataBuffer(index));
        ASSERT(string.GetLength() == (int)m_dictItem.GetLen());

        int spaces_needed = string.GetLength() - value.GetLength();
        TCHAR* string_buffer = string.GetBuffer();

        if( spaces_needed <= 0 )
        {
            _tmemcpy(string_buffer, (LPCTSTR)value, string.GetLength());
        }

        else
        {
            _tmemcpy(string_buffer, (LPCTSTR)value, value.GetLength());
            _tmemset(string_buffer + value.GetLength(), _T(' '), spaces_needed);
        }

        string.ReleaseBuffer(m_dictItem.GetLen());

        RunPostSetValueTasks(index);
    }

    /// <summary>
    /// Sets the string case item from a buffer that contains the exact number
    /// of characters needed to fill the string.
    /// </summary>
    void SetFixedWidthValue(CaseItemIndex& index, const TCHAR* value) const
    {
        CString& string = *static_cast<CString*>(GetDataBuffer(index));
        _tmemcpy(string.GetBuffer(), value, string.GetLength());
        string.ReleaseBuffer(m_dictItem.GetLen());

        RunPostSetValueTasks(index);
    }

    void OutputFixedValue(const CaseItemIndex& index, TCHAR* text_buffer) const override
    {
        _tmemcpy(text_buffer, GetValue(index), m_dictItem.GetLen());
    }
};
