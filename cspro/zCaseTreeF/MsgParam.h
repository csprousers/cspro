#pragma once


struct CMsgParam
{
    HTREEITEM hParam = nullptr;
    bool bParam = false;
    int iParam = -1;
    CString csParam;
    RECT rect;
    bool bMustBeDestroyedAfterLastCatchMessage = false;

    CArray<DWORD_PTR, DWORD_PTR> dwArrayParam;
};
