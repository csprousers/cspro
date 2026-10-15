#pragma once

/////////////////////////////////////////////////////////////////////////////
// CMDlgBar dialog

#include <zFormF/FrmTrCtl.h>
#include <zOrderF/OrdTrCtl.h>
#include <zTableF/TabTrCtl.h>
#include <CSPro/ObjTCtrl.h>
#include <CSPro/MTabCtl.h>

class DynamicLayoutControlResizer;


class CMDlgBar : public COXSizeDialogBar
{
    DECLARE_DYNAMIC(CMDlgBar)

public:
    CMDlgBar();
    ~CMDlgBar();

    BOOL Create(CWnd* pParentWnd, UINT nStyle);

    void UpdateTabs();
    void SelectTab(CString sTabName, int iTabMode);      //Selects the tab from the tab control
    void SelectTab(CWnd* pWnd);      //Selects the tab from the tab control by ptr to window for the tab contents
    void UpdateTrees();
    void GetTabNames(CStringArray& aNames) const;
    void GetTabWindows(CArray<CWnd*>& aWnds);
    CWnd* NameToWindow(LPCTSTR lpszName);
    CIMSAString WindowToName(CWnd* pWnd) const;

    CFormTreeCtrl   m_FormTree;
    COrderTreeCtrl  m_OrderTree;
    CTabTreeCtrl    m_TableTree;
    CDDTreeCtrl     m_DictTree;
    CMTabCtl        m_tabCtl;
    CObjTreeCtrl    m_ObjTree;

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    void OnSizedOrDocked(int cx, int cy, BOOL bFloating, int flags) override;

    afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);

private:
    std::unique_ptr<DynamicLayoutControlResizer> m_dynamicLayoutControlResizer;
};
