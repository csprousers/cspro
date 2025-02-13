#pragma once
// MsgParam.h: interface for the CMsgParam class.
//
//////////////////////////////////////////////////////////////////////

#include <zCaseTreeF/zCaseTreeF.h>


class ZCASETREEF_API CMsgParam  : public CObject
{
public:
        CMsgParam();
        virtual ~CMsgParam();

public:

        HTREEITEM       hParam;
        bool            bParam;
    //DWORD       dwParam;
    int         iParam;
    CString     csParam;
        RECT            rect;
        bool            bMustBeDestroyedAfterLastCatchMessage;

    CArray<DWORD,DWORD> dwArrayParam;
};
