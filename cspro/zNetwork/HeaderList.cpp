#include "stdafx.h"
#include "HeaderList.h"

CString HeaderList::value(const CString& headerName) const
{
    for (const auto& i : *this) {
        int colon = i.Find(_T(':'));
        if (colon >= 0 && i.Left(colon).CompareNoCase(headerName) == 0) {
            return i.Mid(colon + 1).Trim();
        }
    }
    return CString();
}

void HeaderList::update(const CString& headerName, const CString& newValue)
{
    for (auto& i : m_list) {
        int colon = i.Find(_T(':'));
        if (colon >= 0 && i.Left(colon).CompareNoCase(headerName) == 0) {
            i = i.Left(colon + 1) + newValue;
        }
    }
}
