#pragma once
#include <afxlinkctrl.h>

class PopupInfoLinkCtrl : public CMFCLinkCtrl
{
public:
    PopupInfoLinkCtrl(CString popup_info_text)
        :   m_popupInfoText(popup_info_text)
    {
    }

    virtual ~PopupInfoLinkCtrl() { }

protected:
    virtual BOOL PreTranslateMessage(MSG* pMsg) override
    {
        if( pMsg->message == WM_LBUTTONUP || ( pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN ) )
        {
            MessageBox(m_popupInfoText);
            return TRUE;
        }

        return CMFCLinkCtrl::PreTranslateMessage(pMsg);
    }

private:
    CString m_popupInfoText;
};
