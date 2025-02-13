// MTabCtl.cpp : implementation file
//

#include "StdAfx.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const int MAXTABCHAR = 126 ;
//const char* const DICT_TAB_LABEL = "Dictionaries";
//const char* const FORM_TAB_LABEL = "Forms";
//const char* const TABLE_TAB_LABEL = "Tables";


/////////////////////////////////////////////////////////////////////////////
// CMTabCtl

CMTabCtl::CMTabCtl()
{
}

CMTabCtl::~CMTabCtl()
{
}


BEGIN_MESSAGE_MAP(CMTabCtl, CTabCtrl)
    //{{AFX_MSG_MAP(CMTabCtl)
    ON_NOTIFY_REFLECT(TCN_SELCHANGE, OnSelchange)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMTabCtl message handlers

void CMTabCtl::OnSelchange(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
    // TODO: Add your control notification handler code here
    //Get the current selection and the correspondg
    int iCurSel = GetCurSel();


    CMDlgBar* pBar = (CMDlgBar*)GetParent();

    if(iCurSel == 0 ) {
        //then this is the first tba which is 'Object tab"
        pBar->m_ObjTree.ShowWindow(SW_SHOW);
        pBar->m_FormTree.ShowWindow(SW_HIDE);
        pBar->m_OrderTree.ShowWindow(SW_HIDE);
        pBar->m_TableTree.ShowWindow(SW_HIDE);
        pBar->m_DictTree.ShowWindow(SW_HIDE);
    }
    else {
        TCITEM tabItem;
        tabItem.mask = TCIF_TEXT  ;
        TCHAR szText[MAXTABCHAR];
        _tmemset(szText,_T('\0'),MAXTABCHAR);
        tabItem.pszText = szText;
        tabItem.cchTextMax = MAXTABCHAR;

        GetItem(iCurSel,&tabItem);
        CString sDict(DICT_TAB_LABEL);
        CString sForm(FORM_TAB_LABEL);
        CString sOrder(ORDER_TAB_LABEL);
        CString sTable(TABLE_TAB_LABEL);

        if(sDict.CompareNoCase(tabItem.pszText) == 0) {
            pBar->m_DictTree.ShowWindow(SW_SHOW);
            pBar->m_ObjTree.ShowWindow(SW_HIDE);
            pBar->m_FormTree.ShowWindow(SW_HIDE);
            pBar->m_OrderTree.ShowWindow(SW_HIDE);
            pBar->m_TableTree.ShowWindow(SW_HIDE);
        }

        else if(sForm.CompareNoCase(tabItem.pszText) == 0 ) {
            pBar->m_FormTree.ShowWindow(SW_SHOW);
            pBar->m_OrderTree.ShowWindow(SW_HIDE);
            pBar->m_TableTree.ShowWindow(SW_HIDE);
            pBar->m_DictTree.ShowWindow(SW_HIDE);
            pBar->m_ObjTree.ShowWindow(SW_HIDE);
        }

        else if(sOrder.CompareNoCase(tabItem.pszText) == 0 ) {
            pBar->m_OrderTree.ShowWindow(SW_SHOW);
            pBar->m_FormTree.ShowWindow(SW_HIDE);
            pBar->m_TableTree.ShowWindow(SW_HIDE);
            pBar->m_DictTree.ShowWindow(SW_HIDE);
            pBar->m_ObjTree.ShowWindow(SW_HIDE);
        }

        else if(sTable.CompareNoCase(tabItem.pszText) == 0 ) {
            pBar->m_TableTree.ShowWindow(SW_SHOW);
            pBar->m_FormTree.ShowWindow(SW_HIDE);
            pBar->m_OrderTree.ShowWindow(SW_HIDE);
            pBar->m_DictTree.ShowWindow(SW_HIDE);
            pBar->m_ObjTree.ShowWindow(SW_HIDE);
        }
    }

    *pResult = 0;
}

void CMTabCtl::UpdateTabs()
{
    //Get the selected tab item
    int iSel = GetCurSel();
    //Get the parent dialog and go thru the controls if the
    //tree has items show the tab else delete the tab
    CMDlgBar* pBar = (CMDlgBar*)GetParent();

    //Assume the root is the dictionary label it self
    CTreeCtrl& dictTree = pBar->m_DictTree;
    HTREEITEM hRoot = dictTree.GetRootItem();

    if(hRoot){
        int iIndex = GetItemIndex(DICT_TAB_LABEL);
        if(iIndex == -1) {
            InsertItem(1,DICT_TAB_LABEL,1);
        }
    }
    else{
        int iIndex = GetItemIndex(DICT_TAB_LABEL);
        if (iIndex != -1) {
            DeleteItem(iIndex);
        }

    }

    //Assume the root is the Form label it self for form tree
    CTreeCtrl& formTree = pBar->m_FormTree;
    hRoot = formTree.GetRootItem();
    if(hRoot){
        int iIndex = GetItemIndex(FORM_TAB_LABEL);
        if(iIndex == -1) {
            InsertItem(2,FORM_TAB_LABEL,3);
        }
    }
    else{
        int iIndex = GetItemIndex(FORM_TAB_LABEL);
        if (iIndex != -1) {
            DeleteItem(iIndex);
        }

    }

    //Assume the root is the Order label it self for order tree
    CTreeCtrl& orderTree = pBar->m_OrderTree;
    hRoot = orderTree.GetRootItem();
    if(hRoot){
        int iIndex = GetItemIndex(ORDER_TAB_LABEL);
        if(iIndex == -1) {
            InsertItem(2,ORDER_TAB_LABEL,4);
        }
    }
    else{
        int iIndex = GetItemIndex(ORDER_TAB_LABEL);
        if (iIndex != -1) {
            DeleteItem(iIndex);
        }
    }

    //Assume the root is the Table label it self for table tree
    CTreeCtrl& tableTree = pBar->m_TableTree;
    hRoot = tableTree.GetRootItem();
    if(hRoot){
        int iIndex = GetItemIndex(TABLE_TAB_LABEL);
        if(iIndex == -1) {
            InsertItem(2,TABLE_TAB_LABEL,2);
        }
    }
    else{
        int iIndex = GetItemIndex(TABLE_TAB_LABEL);
        if (iIndex != -1) {
            DeleteItem(iIndex);
        }
    }


    if(iSel < GetItemCount()) {
        SetCurSel(iSel);
    }
    else {
        //Sending a message is required to simulate tab change
        int iIndex = GetItemCount() - 1;
        SetCurSel(iIndex);
        NMHDR pNMHDR ;
        pNMHDR.hwndFrom =this->GetSafeHwnd();
        pNMHDR.code = TCN_SELCHANGE ;
        pNMHDR.idFrom = iIndex ;

        this->SendMessage(WM_NOTIFY,0,(LPARAM)&pNMHDR);
    }
}


/*****************************************************************************
return the item which has the the given text else return -1
******************************************************************************/
int CMTabCtl::GetItemIndex(const CString& sString) const{
    int iRet = -1;
    int iCount = GetItemCount();
    for (int iIndex = 0; iIndex < iCount ; iIndex ++) {
        TCITEM tcItem;
        TCHAR szText[MAXTABCHAR];
        _tmemset(szText,_T('\0'),MAXTABCHAR);
        tcItem.mask = TCIF_TEXT;
        tcItem.pszText = szText;
        tcItem.cchTextMax = MAXTABCHAR;
        GetItem(iIndex,&tcItem);
        if(sString.CompareNoCase(tcItem.pszText) == 0){
            iRet = iIndex;
            break;
        }
    }

    return iRet;

}

void  CMTabCtl::InitImageList(void)
{
    //m_cImageList.Create(32, 32, ILC_COLOR32| ILC_MASK, 0, 4); // x,y of icon, flags, initial sz, grow by

    // Create the full-color image list
    // cx, cy = your icon width & height
    // You could also use ILC_COLOR24 rather than ILC_COLOR32
    m_cImageList.Create(16, 16, ILC_MASK | ILC_COLOR32, 0, 0);

    HICON icon0 = AfxGetApp()->LoadIcon (IDR_MAINFRAME);        // 0
    HICON icon1 = AfxGetApp()->LoadIcon (IDR_DICT_FRAME);       // 1
    HICON icon2 = AfxGetApp()->LoadIcon (IDR_TABLE_FRAME);      // 2
    HICON icon3 = AfxGetApp()->LoadIcon (IDR_FORM_FRAME);       // 3
    HICON icon4 = AfxGetApp()->LoadIcon (IDR_ORDER_FRAME);      // 4

    CBitmap bmp;
    // Load your imagelist bitmap (bmp) here, however
    //   you feel like (e.g. CBitmap::LoadBitmap)
    CClientDC clientDC(this);
    CDC dc;
    int cx = 16, cy = 16;
    dc.CreateCompatibleDC(&clientDC);
    bmp.CreateCompatibleBitmap(&clientDC, cx, cy);
    CBitmap* pOldBmp = (CBitmap*)dc.SelectObject(&bmp);

    ::DrawIconEx( dc.GetSafeHdc(), 0, 0, icon0, cx, cy, 0, (HBRUSH)RGB(192, 192, 192), DI_NORMAL);

    dc.SelectObject( pOldBmp );

    dc.DeleteDC();


    COLORREF rgbTransparentColor = RGB(255,255,255);
    // Set up your transparent color as appropriate

    // Add the bitmap into the image list
    m_cImageList.Add(&bmp, rgbTransparentColor);

    //m_cImageList.Add (icon0);
    m_cImageList.Add (icon1);
    m_cImageList.Add (icon2);
    m_cImageList.Add (icon3);
    m_cImageList.Add (icon4);

    SetImageList (&m_cImageList);
}


