//***************************************************************************
//  File name: TreePrps.cpp
//
//  Description:
//       Tree properties dialog.  Property sheet like dialog with a tree control on
//  the left for choosing between the pages.  Pages are actually just CDialogs to
//  allow for use of same dialog class as page or as seperate modal dialog.
//
//***************************************************************************

#include "StdAfx.h"
#include "TreePrps.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTreePropertiesDlg dialog

/////////////////////////////////////////////////////////////////////////////////
//                      CTreePropertiesDlg::CTreePropertiesDlg
// Constructor
/////////////////////////////////////////////////////////////////////////////////
CTreePropertiesDlg::CTreePropertiesDlg(const CString& sTitle,
    std::optional<unsigned> dialog_id_override/* = std::nullopt*/, CWnd* pParent /*=NULL*/)
    : m_pCurrDlg(NULL),
      m_sDlgTitle(sTitle),
      CDialog(dialog_id_override.value_or(IDD_TREE_PROP_DLG), pParent)
{
}

/////////////////////////////////////////////////////////////////////////////////
//                      CTreePropertiesDlg::DoDataExchange
/////////////////////////////////////////////////////////////////////////////////
void CTreePropertiesDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_PROP_TREE, m_treeCtrl);
    DDX_Control(pDX, IDC_PAGE_CAPTION, m_captionCtrl);
}

/////////////////////////////////////////////////////////////////////////////////
//                      CTreePropertiesDlg Message Map
/////////////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CTreePropertiesDlg, CDialog)
    ON_NOTIFY(TVN_SELCHANGED, IDC_PROP_TREE, OnSelchangedTree)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTreePropertiesDlg message handlers

/////////////////////////////////////////////////////////////////////////////////
//                      CTreePropertiesDlg::OnInitDialog
/////////////////////////////////////////////////////////////////////////////////
BOOL CTreePropertiesDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    SetWindowText(m_sDlgTitle);

    // get rect in which to place pages (rect of placeholder static item in dlg template)
    CWnd* pPlaceholder = GetDlgItem(IDC_PAGE_PLACEHOLDER);
    ASSERT(pPlaceholder != NULL);
    pPlaceholder->GetWindowRect(&m_pageRect);
    ScreenToClient(&m_pageRect);
    pPlaceholder->ShowWindow(SW_HIDE); // don't show the placeholder static

    // pages added before dlg window was created need to be added for real
    for (int i = 0; i < m_deferAddPages.GetSize(); ++i) {
        const DeferAddStruct& as = m_deferAddPages[i];
        AddPage(as.pPage, as.sCaption, as.pParent, as.pInsertAfter);
    }

    // set the initial page
    if (m_pCurrDlg != NULL) {
        SetPage(m_pCurrDlg);
    }

    // expand all top level nodes in tree
    HTREEITEM hCurrent = m_treeCtrl.GetRootItem();
    while (hCurrent)
    {
        m_treeCtrl.Expand(hCurrent, TVE_EXPAND);
        hCurrent = m_treeCtrl.GetNextItem(hCurrent, TVGN_NEXT);
    }

    return TRUE;  // return TRUE  unless you set the focus to a control
}

/////////////////////////////////////////////////////////////////////////////////
//                      CTreePropertiesDlg::OnSelchangedTree
// Called when selection in tree control changes.  Need to change page.
/////////////////////////////////////////////////////////////////////////////////
void CTreePropertiesDlg::OnSelchangedTree(NMHDR* pNMHDR, LRESULT* pResult)
{
    NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;

    HTREEITEM hItem = pNMTreeView->itemNew.hItem;
    ASSERT(hItem != NULL);

    // page is stored in item data of tree node
    CDialog* pPage = (CDialog*) m_treeCtrl.GetItemData(hItem);
    if (pPage != NULL) {
        SetPage(pPage);
    }

    *pResult = 0;
}

/////////////////////////////////////////////////////////////////////////////////
//                      CTreePropertiesDlg::AddPage
// Add new page.
/////////////////////////////////////////////////////////////////////////////////
void CTreePropertiesDlg::AddPage(CDialog* pDlg,
             LPCTSTR sCaption,
             CDialog* pParent /* = NULL */,
             CDialog* pInsertAfter /* = NULL */)
{
    if (IsInitialized()) {

        bool bFirstItem = (m_treeCtrl.GetRootItem() == NULL);

        // if the window has been created - add the page

        TVINSERTSTRUCT item;

        // find parent and insert after items in tree
        item.hParent = (pParent == NULL ? TVI_ROOT : FindItemByPage(pParent));
        item.hInsertAfter = (pInsertAfter == NULL ? TVI_LAST : FindItemByPage(pInsertAfter));
        ASSERT(pParent == NULL || pInsertAfter == NULL ||
               m_treeCtrl.GetParentItem(item.hInsertAfter) == item.hParent);

        // add new entry to tree control
        item.item.mask = TVIF_PARAM | TVIF_TEXT;
        item.item.pszText = const_cast<LPTSTR>(sCaption);
        item.item.cchTextMax = _tcslen(sCaption);
        item.item.lParam = (DWORD) pDlg; // store page pointer in tree node (see SetPage)
        m_treeCtrl.InsertItem(&item);

        // turn regular modal dialog into child window so that it can be displayed
        // as page
        pDlg->ModifyStyle(WS_POPUP | WS_BORDER | WS_SYSMENU | WS_CAPTION | WS_CLIPSIBLINGS,
                    WS_CHILD);
        pDlg->ModifyStyleEx(WS_EX_DLGMODALFRAME, 0);

        // Get rect for the new page -- check if it fits or if we need to resize
        CRect newPageRect;
        pDlg->GetWindowRect(&newPageRect);
        newPageRect.MoveToXY(m_pageRect.TopLeft());

        const int iXtraSpace = 4; // need a little buffer between page and main dlg
        newPageRect.bottom += iXtraSpace;
        newPageRect.right += iXtraSpace;

        // if this is the first page then shrink or grow to fit it
        if (bFirstItem) {


            // make sure new page rect is big enough to contain
            // to leave space for help button on right
            CWnd* pCtrl = GetDlgItem(ID_HELP);
            CRect rectCtrl;
            pCtrl->GetWindowRect(&rectCtrl);
            ScreenToClient(&rectCtrl);
            if (rectCtrl.right > newPageRect.right) {
                newPageRect.right = rectCtrl.right;
            }

            ResizeDlg(newPageRect);

        }
        else if (newPageRect.right > m_pageRect.right ||
                 newPageRect.bottom > m_pageRect.bottom) {
            // already have shrunk to an earlier page, only grow if we need to
            newPageRect.right = std::max(newPageRect.right, m_pageRect.right);
            newPageRect.bottom = std::max(newPageRect.bottom, m_pageRect.bottom);
            ResizeDlg(newPageRect);
        }

        pDlg->SetParent(this);
        pDlg->MoveWindow(m_pageRect.left, m_pageRect.top, m_pageRect.Width(), m_pageRect.Height());

        // hide OK, Cancel and Help buttons on the page dialog if they exist (since we have these already)
        CWnd* pOK = pDlg->GetDlgItem(IDOK);
        if (pOK != NULL) {
            pOK->ShowWindow(SW_HIDE);
        }
        CWnd* pCancel = pDlg->GetDlgItem(IDCANCEL);
        if (pCancel != NULL) {
            pCancel->ShowWindow(SW_HIDE);
        }
        CWnd* pHelp = pDlg->GetDlgItem(ID_HELP);
        if (pHelp != NULL) {
            pHelp->ShowWindow(SW_HIDE);
        }
    }
    else {

        // can't add page yet since this window (and tree ctrl) have not been created yet.
        // store the info and we will add in OnInitDialog
        DeferAddStruct as;
        as.pPage = pDlg;
        as.pParent = pParent;
        as.pInsertAfter = pInsertAfter;
        as.sCaption = sCaption;
        m_deferAddPages.Add(as);
    }
}

/////////////////////////////////////////////////////////////////////////////////
//                      CTreePropertiesDlg::SetPage
// Set currently displayed page.
/////////////////////////////////////////////////////////////////////////////////
void CTreePropertiesDlg::SetPage(CDialog* pPage)
{
    CDialog* pOldPage = m_pCurrDlg;

    if (m_pCurrDlg != NULL && IsInitialized()) {
        // hide the old one
        m_pCurrDlg->ShowWindow(SW_HIDE);
    }

    m_pCurrDlg = pPage;

    OnPageChange(pOldPage, m_pCurrDlg);

    if (IsInitialized() && m_pCurrDlg != NULL) {
        ASSERT_VALID(m_pCurrDlg);

        // show the new one
        m_pCurrDlg->ShowWindow(SW_SHOW);
        m_pCurrDlg->SetFocus();

        // select the appropriate tree node to keep tree in synch w. curr page
        HTREEITEM hItem = FindItemByPage(pPage);
        ASSERT(hItem != NULL);
        m_treeCtrl.SelectItem(hItem);

        // set text of the caption to the caption for the new current page
        CWnd* pCaption = GetDlgItem(IDC_PAGE_CAPTION);
        ASSERT_VALID(pCaption);
        pCaption->SetWindowText(m_treeCtrl.GetItemText(hItem));
    }
}

bool CheckTreeItemMatchesPage(CTreeCtrl& treeCtrl, HTREEITEM hItem, void* pPage)
{
    CDialog* pItemPage = (CDialog*) treeCtrl.GetItemData(hItem);
    if (pPage == pItemPage) {
        return false; // found it
    }
    return true; // continue search
}

/////////////////////////////////////////////////////////////////////////////////
//                      CTreePropertiesDlg::FindItemByPage
// Find node in tree for a page given pointer to page.
/////////////////////////////////////////////////////////////////////////////////
HTREEITEM CTreePropertiesDlg::FindItemByPage(CDialog* pPage)
{
    return ForEachTreeItem(&CheckTreeItemMatchesPage, pPage);
}

bool OnOKTreeItemPage(CTreeCtrl& treeCtrl, HTREEITEM hItem, void*)
{
    CDialog* pPage = (CDialog*) treeCtrl.GetItemData(hItem);
    if (pPage != NULL) {
        ASSERT_VALID(pPage);
        pPage->SendMessage(WM_COMMAND, IDOK, 0);
    }
    return true; // continue search
}


/////////////////////////////////////////////////////////////////////////////////
//                      CTreePropertiesDlg::OnOK
// Called when user hits ok button on main dlg.
// Override to pass the OK onto all the pages so that data exchange is performed.
/////////////////////////////////////////////////////////////////////////////////
void CTreePropertiesDlg::OnOK()
{
    CDialog* pCurrPage = m_pCurrDlg;
    SetPage(NULL); // trigger page change
    m_pCurrDlg = pCurrPage;

    ForEachTreeItem(&OnOKTreeItemPage, NULL);

    CDialog::OnOK();

}

/////////////////////////////////////////////////////////////////////////////////
//                      CTreePropertiesDlg::ForEachTreeItem
// Walk the tree to perform ops on each item or to find a particular item.
// Calls user supplied function fn on each tree item it finds.  If fn returns
// false it stops the search and returns the item handle, otherwise it
// continues walking the tree.
/////////////////////////////////////////////////////////////////////////////////
HTREEITEM CTreePropertiesDlg::ForEachTreeItem(bool fn(CTreeCtrl&, HTREEITEM, void*), void* pUserData)
{
    HTREEITEM hItem = m_treeCtrl.GetRootItem();
    if (hItem != NULL) {
        HTREEITEM hLastItem;

        do {

            // walk children first
            do {
                hLastItem = hItem;
                if (!(*fn)(m_treeCtrl, hItem, pUserData)) {
                    return hItem;
                }
                hItem = m_treeCtrl.GetChildItem(hLastItem);
            } while ( hItem != NULL );

            // move to next sibling, if no sibling move back up to next sibling of parent
            do {
                hItem = m_treeCtrl.GetNextSiblingItem(hLastItem);
                if (hItem == NULL) {
                    hLastItem = m_treeCtrl.GetParentItem(hLastItem);
                }
            } while (hItem == NULL && hLastItem != NULL);


        } while ( hItem != NULL );
    }

    return NULL;
}

/////////////////////////////////////////////////////////////////////////////////
//                      CTreePropertiesDlg::IsInitialized
// Return true if dialog has been initialized (window exists).
/////////////////////////////////////////////////////////////////////////////////
bool CTreePropertiesDlg::IsInitialized()
{
    return ::IsWindow(m_hWnd) == TRUE;
}

/////////////////////////////////////////////////////////////////////////////////
//                      CTreePropertiesDlg::OnPageChange
// override to do updates when user changes page
/////////////////////////////////////////////////////////////////////////////////
void CTreePropertiesDlg::OnPageChange(CDialog* , CDialog* )
{
}

/////////////////////////////////////////////////////////////////////////////////
//                      CTreePropertiesDlg::ResizeDlg
// move dialog controls to fit new size of page rect
/////////////////////////////////////////////////////////////////////////////////
void CTreePropertiesDlg::ResizeDlg(const CRect& newPageRect)
{
    CRect rectCtrl;
    CWnd* pCtrl = NULL;

    const int dy = newPageRect.bottom - m_pageRect.bottom;
    const int dx = newPageRect.right - m_pageRect.right;

    m_treeCtrl.GetWindowRect(&rectCtrl);
    ScreenToClient(&rectCtrl);
    rectCtrl.bottom += dy;
    m_treeCtrl.MoveWindow(&rectCtrl, FALSE);

    m_captionCtrl.GetWindowRect(&rectCtrl);
    ScreenToClient(&rectCtrl);
    rectCtrl.right += dx;
    m_captionCtrl.MoveWindow(&rectCtrl, FALSE);

    pCtrl = GetDlgItem(IDOK);
    pCtrl->GetWindowRect(&rectCtrl);
    ScreenToClient(&rectCtrl);
    rectCtrl.OffsetRect(0, dy);
    pCtrl->MoveWindow(&rectCtrl, FALSE);

    pCtrl = GetDlgItem(IDCANCEL);
    pCtrl->GetWindowRect(&rectCtrl);
    ScreenToClient(&rectCtrl);
    rectCtrl.OffsetRect(0, dy);
    pCtrl->MoveWindow(&rectCtrl, FALSE);

    pCtrl = GetDlgItem(ID_HELP);
    pCtrl->GetWindowRect(&rectCtrl);
    ScreenToClient(&rectCtrl);
    rectCtrl.OffsetRect(0, dy);
    pCtrl->MoveWindow(&rectCtrl, FALSE);

    CRect winRect;
    GetWindowRect(&winRect);
    winRect.right += dx;
    winRect.bottom += dy;
    MoveWindow(&winRect, TRUE);

    m_pageRect = newPageRect;
}

