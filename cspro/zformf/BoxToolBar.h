#pragma once

class CFormChildWnd;


class BoxToolbar : public CToolBar
{
public:
    BoxToolbar(CFormChildWnd* pFormCW);

    void SetFormChildWnd(CFormChildWnd* pFormCW) { m_pFormCW = pFormCW; }

protected:
    DECLARE_MESSAGE_MAP()

    afx_msg void OnWindowPosChanging(LPWINDOWPOS lpWndPos);

private:
    CFormChildWnd* m_pFormCW;
};
