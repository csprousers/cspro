#pragma once

#include <zDesignerF/UnfloatableDialogBar.h>
#include <zDictF/DictPropertyGridCtrl.h>


class CDictDialogBar : public UnfloatableDialogBar
{
public:
    CDictDialogBar();

    bool CreateAndDock(CFrameWnd* pFrameWnd);

    CDictPropertyGridCtrl* GetPropCtrl() { return &m_wndPropGridCtrl; }

protected:
    DECLARE_MESSAGE_MAP()

    int OnCreate(LPCREATESTRUCT lpCreateStruct);
    BOOL OnInitDialog() override;

private:
    CDictPropertyGridCtrl m_wndPropGridCtrl;
public:
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
};
