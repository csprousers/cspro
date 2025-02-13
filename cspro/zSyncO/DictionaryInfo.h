#pragma once

#include <zSyncO/zSyncO.h>

struct SYNC_API DictionaryInfo
{
    CString m_name;
    CString m_label;
    int m_caseCount;

    DictionaryInfo(CString name, CString label, int caseCount) :
        m_name(name),
        m_label(label),
        m_caseCount(caseCount)
    {}

};
