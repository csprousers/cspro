#pragma once

#include <zDesignerF/UnfloatableDialogBar.h>
#include <zDictF/DictPropertyGridCtrl.h>


class CDictDialogBar : public UnfloatableDialogBar
{
public:
    CDictDialogBar();

    bool CreateAndDock(CFrameWnd* const frame_wnd);

    CDictPropertyGridCtrl& GetPropCtrl() noexcept { return m_wndPropGridCtrl; }

protected:
    DECLARE_MESSAGE_MAP()

    int OnCreate(LPCREATESTRUCT lpCreateStruct);
    void OnSize(UINT nType, int cx, int cy);

    BOOL OnInitDialog() override;

    void OnLButtonUp(UINT nFlags, CPoint point);

private:
    CDictPropertyGridCtrl m_wndPropGridCtrl;
};
