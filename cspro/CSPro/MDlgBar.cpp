#include "StdAfx.h"
#include "MDlgBar.h"
#include <zUtilO/DynamicLayoutControlResizer.h>


IMPLEMENT_DYNAMIC(CMDlgBar, COXSizeDialogBar)


BEGIN_MESSAGE_MAP(CMDlgBar, COXSizeDialogBar)
    ON_WM_LBUTTONDBLCLK()
    ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()


CMDlgBar::CMDlgBar()
    :   COXSizeDialogBar(SZBARF_STDMOUSECLICKS)
{
}


CMDlgBar::~CMDlgBar() = default;


BOOL CMDlgBar::Create(CWnd* const pParentWnd, const UINT nStyle)
{
    ASSERT(( m_Style & SZBARF_DLGAUTOSIZE ) == 0 );

    if( !__super::Create(pParentWnd, IDD_DIALOGBAR, nStyle, 0) )
        return FALSE;

    // by default, DynamicLayoutControlResizer uses the client window for its
    // sizing calculation, but doing this here in Create (as opposed to OnSizedOrDocked),
    // using the window rectangle seems to work better for these calculations
    CRect rect;
    GetWindowRect(rect);
    const CSize size = rect.Size();
    m_dynamicLayoutControlResizer = std::make_unique<DynamicLayoutControlResizer>(*this, &size);

    m_dynamicLayoutControlResizer->Add(IDC_OBJTREE, SizingDirection::XY);
    m_dynamicLayoutControlResizer->Add(IDC_DICTTREE, SizingDirection::XY);
    m_dynamicLayoutControlResizer->Add(IDC_FORMTREE, SizingDirection::XY);
    m_dynamicLayoutControlResizer->Add(IDC_ORDERTREE, SizingDirection::XY);
    m_dynamicLayoutControlResizer->Add(IDC_TABLETREE, SizingDirection::XY);
    m_dynamicLayoutControlResizer->Add(IDC_MEASURETAB, SizingDirection::X);
    m_dynamicLayoutControlResizer->Add(IDC_MEASURETAB, MovingDirection::Y);

    return TRUE;
}


void CMDlgBar::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_FORMTREE, m_FormTree);
    DDX_Control(pDX, IDC_ORDERTREE, m_OrderTree);
    DDX_Control(pDX, IDC_TABLETREE, m_TableTree);
    DDX_Control(pDX, IDC_DICTTREE, m_DictTree);
    DDX_Control(pDX, IDC_MEASURETAB, m_tabCtl);
    DDX_Control(pDX, IDC_OBJTREE, m_ObjTree);
}


BOOL CMDlgBar::OnInitDialog()
{
    UpdateData(FALSE);

    m_DictTree.InitImageList();
    m_DictTree.ShowWindow(SW_HIDE);

    m_FormTree.InitImageList();
    m_FormTree.ShowWindow(SW_HIDE);
    m_FormTree.SetDDTreeCtrl(&m_DictTree);

    m_OrderTree.InitImageList();
    m_OrderTree.ShowWindow(SW_HIDE);
    m_OrderTree.SetDDTreeCtrl(&m_DictTree);

    m_TableTree.InitImageList();
    m_TableTree.ShowWindow(SW_HIDE);
    m_TableTree.SetDDTreeCtrl(&m_DictTree);

    m_tabCtl.InitImageList();
    m_tabCtl.InsertItem(0,OBJECT_TAB_LABEL,0);

    m_ObjTree.InitImageList();

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}


void CMDlgBar::OnSizedOrDocked(int cx, int cy, BOOL bFloating, int flags)
{
    ASSERT(( m_Style & SZBARF_DLGAUTOSIZE ) == 0 );

    __super::OnSizedOrDocked(cx, cy, bFloating, flags);

    if( m_dynamicLayoutControlResizer != nullptr )
        m_dynamicLayoutControlResizer->OnSize(cx, cy);
}


void CMDlgBar::UpdateTabs()
{
    //check if a dictionary is open and if the tab control has dictionary tab
    //If the tab control has no dictionary tab  add it
    //If a dictionary is not open delete the dictionary tab if it exists

    //check if a Form  is open and if the tab control has Form tab
    //If the tab control has no Form tab  add it
    //If a Form is not open delete the form tab if it exists

    //Same with the other possible tabs
    m_tabCtl.UpdateTabs();

}


void CMDlgBar::SelectTab(CString sTabName, int iTabMode)
{
    // Set view names
    if (iTabMode > 0) {
        if (SharedSettings::ViewNamesInTree()) {
            if (iTabMode == 1) {
                ((CCSProApp*)AfxGetApp())->OnViewNames();
            }
        }
        else {
            if (iTabMode == 2) {
                ((CCSProApp*)AfxGetApp())->OnViewNames();
            }
        }
    }

    int iIndex = m_tabCtl.GetItemIndex(sTabName);

    if(iIndex == -1 ) {
        iIndex  =0;
    }
    m_tabCtl.SetCurSel(iIndex);
    NMHDR pNMHDR ;
    pNMHDR.hwndFrom =m_tabCtl.GetSafeHwnd();
    pNMHDR.code = TCN_SELCHANGE ;
    pNMHDR.idFrom = iIndex ;

    m_tabCtl.SendMessage(WM_NOTIFY,0,(LPARAM)&pNMHDR);
    UpdateTrees();
}


void CMDlgBar::SelectTab(CWnd* pWnd)
{
    SelectTab(WindowToName(pWnd), 0);
}


void CMDlgBar::GetTabNames(CStringArray& aNames) const
{
    const int iCount = m_tabCtl.GetItemCount();
    for (int iIndex = 0; iIndex < iCount ; iIndex ++) {
        TCITEM tcItem;
        TCHAR szText[256];
        _tmemset(szText,_T('\0'),256);
        tcItem.mask = TCIF_TEXT;
        tcItem.pszText = szText;
        tcItem.cchTextMax = 256;
        m_tabCtl.GetItem(iIndex,&tcItem);
        aNames.Add(szText);
    }
}


void CMDlgBar::GetTabWindows(CArray<CWnd*>& aWnds)
{
    CStringArray aNames;
    GetTabNames(aNames);
    for (int i = 0; i < aNames.GetCount(); ++i) {
        aWnds.Add(NameToWindow(aNames.GetAt(i)));
    }
}


CWnd* CMDlgBar::NameToWindow(LPCTSTR lpszName)
{
    CString sName(lpszName);
    if (sName == DICT_TAB_LABEL) {
        return &m_DictTree;
    }
    else if (sName == FORM_TAB_LABEL) {
        return &m_FormTree;
    }
    else if (sName == ORDER_TAB_LABEL) {
        return &m_OrderTree;
    }
    else if (sName == TABLE_TAB_LABEL) {
        return &m_TableTree;
    }
    else if (sName == OBJECT_TAB_LABEL) {
        return &m_ObjTree;
    }

    return NULL;
}


CIMSAString CMDlgBar::WindowToName(CWnd* pWnd) const
{
    CIMSAString sName;
    if (pWnd == &m_FormTree) {
        sName = FORM_TAB_LABEL;
    }
    else if (pWnd == &m_OrderTree) {
        sName = ORDER_TAB_LABEL;
    }
    else if (pWnd == &m_DictTree) {
        sName = DICT_TAB_LABEL;
    }
    else if (pWnd == &m_TableTree) {
        sName = TABLE_TAB_LABEL;
    }
    else if (pWnd == &m_ObjTree) {
        sName = OBJECT_TAB_LABEL;
    }

    return sName;
}


void CMDlgBar::UpdateTrees()
{
    m_DictTree.Invalidate(TRUE);
    m_FormTree.Invalidate(TRUE);
    m_OrderTree.Invalidate(TRUE);
    m_TableTree.Invalidate(TRUE);
    m_ObjTree.Invalidate(TRUE);
}


void CMDlgBar::OnLButtonDblClk(UINT /*nFlags*/, CPoint /*point*/)
{
    return; // Do not process double click  to avoid floating toobar
}


void CMDlgBar::OnLButtonDown(UINT /*nFlags*/, CPoint /*point*/)
{
    return;  //Do not process LButton down to avoid dragging the dialogbar away to float it
}
