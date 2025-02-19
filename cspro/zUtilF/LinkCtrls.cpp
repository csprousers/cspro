#include "StdAfx.h"
#include "LinkCtrls.h"


// --------------------------------------------------------------------------
// BaseLinkCtrl
// --------------------------------------------------------------------------

BEGIN_MESSAGE_MAP(BaseLinkCtrl, CMFCLinkCtrl)
    ON_CONTROL_REFLECT_EX(BN_CLICKED, OnClicked)
END_MESSAGE_MAP()


BOOL BaseLinkCtrl::PreTranslateMessage(MSG* const pMsg)
{
    if( ( pMsg->message == WM_LBUTTONUP ) ||
        ( pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN ) )
    {
        OnClicked();
        return TRUE;
    }

    // have CMFCButton, not CMFCLinkCtrl, handle other messages
    return CMFCButton::PreTranslateMessage(pMsg);
}


void BaseLinkCtrl::OnDraw(CDC* const pDC, const CRect& rect, const UINT uiState)
{
    // we will never show a link as visited
    m_bVisited = FALSE;

    return __super::OnDraw(pDC, rect, uiState);
}


BOOL BaseLinkCtrl::OnClicked()
{
    // modified from CMFCLinkCtrl::OnClicked
    ASSERT_VALID(this);

    if( !IsWindowEnabled() )
        return TRUE;

    ASSERT(!m_bDefaultClickProcess);

    OnClick();

    m_bHover = FALSE;
    Invalidate();
    UpdateWindow();

    // don't have CMFCLinkCtrl process this
    return TRUE;
}



// --------------------------------------------------------------------------
// MessagePostingLinkCtrl
// --------------------------------------------------------------------------

MessagePostingLinkCtrl::MessagePostingLinkCtrl(CWnd* const wnd, const UINT message)
    :   m_wnd(wnd),
        m_message(message)
{
    ASSERT(m_wnd != nullptr);
}


void MessagePostingLinkCtrl::OnClick()
{
    ASSERT(m_wnd->GetSafeHwnd() != nullptr);

    m_wnd->PostMessage(m_message);
}



// --------------------------------------------------------------------------
// PopupInfoLinkCtrl
// --------------------------------------------------------------------------

PopupInfoLinkCtrl::PopupInfoLinkCtrl(std::wstring popup_info_text)
    :   m_popupInfoText(std::move(popup_info_text))
{
}


void PopupInfoLinkCtrl::OnClick()
{
    MessageBox(m_popupInfoText.c_str());
}
