#include "StdAfx.h"
#include "EditControls.h"


// --------------------------------------------------------------------------
// CEditWithSelectAll
// --------------------------------------------------------------------------

BOOL CEditWithSelectAll::PreTranslateMessage(MSG* pMsg)
{
    if( pMsg->message == WM_KEYDOWN && pMsg->wParam == 'A' && GetKeyState(VK_CONTROL) < 0 )
    {
        SetSel(0, -1, FALSE);
        return TRUE;
    }

    return __super::PreTranslateMessage(pMsg);
}
