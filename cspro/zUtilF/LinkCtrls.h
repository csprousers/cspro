#pragma once

#include <zUtilF/zUtilF.h>
#include <afxlinkctrl.h>


// --------------------------------------------------------------------------
// BaseLinkCtrl:
//     On click, the virtual method OnClick is called.
//
// MessagePostingLinkCtrl:
//     On click, a message is posted to a window.
//
// PopupInfoLinkCtrl:
//     On click, text is displayed in a message box.
// --------------------------------------------------------------------------


class CLASS_DECL_ZUTILF BaseLinkCtrl : public CMFCLinkCtrl
{
protected:
    DECLARE_MESSAGE_MAP()

    BOOL PreTranslateMessage(MSG* pMsg) override;
    void OnDraw(CDC* pDC, const CRect& rect, UINT uiState) override;

    BOOL OnClicked();

    virtual void OnClick() = 0;
};


class CLASS_DECL_ZUTILF MessagePostingLinkCtrl : public BaseLinkCtrl
{
public:
    MessagePostingLinkCtrl(CWnd* wnd, UINT message);

protected:
    void OnClick() override;

private:
    CWnd* m_wnd;
    UINT m_message;
};


class CLASS_DECL_ZUTILF PopupInfoLinkCtrl : public BaseLinkCtrl
{
public:
    PopupInfoLinkCtrl(std::wstring popup_info_text);

protected:
    void OnClick() override;

private:
    std::wstring m_popupInfoText;
};
