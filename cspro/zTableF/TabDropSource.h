#pragma once

#include <zDictF/DDTrCtl.H>


class TabDropSource : public COleDropSource
{
public:
    SCODE GiveFeedback(DROPEFFECT /*dropEffect*/) override
    {
        ASSERT_VALID(this);

        POINT point;
        GetCursorPos(&point);

        int iRet = CDDTreeCtrl::SetCursor4TabDrop(point, true);

        return ( iRet == DictCursor::Arrow ) ? DRAGDROP_S_USEDEFAULTCURSORS : S_OK;
    }
};
