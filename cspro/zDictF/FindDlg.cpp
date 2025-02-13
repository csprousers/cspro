//***************************************************************************
//  File name: FindDlg.cpp
//
//  Description:
//       Find Dialog Box
//
//  History:    Date       Author   Comment
//              ---------------------------
//              03 Aug 00   bmd     Created for CSPro 2.1
//
//***************************************************************************

#include "StdAfx.h"
#include "FindDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
//
//                        CFindDlg::CFindDlg
//
/////////////////////////////////////////////////////////////////////////////

CFindDlg::CFindDlg(CWnd* pParent /*=NULL*/) : CDialog(IDD_FIND_DLG, pParent) {

    //{{AFX_DATA_INIT(CFindDlg)
    m_bCaseSensitive = FALSE;
    m_csSearchText = _T("");
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CFindDlg::Create
//
/////////////////////////////////////////////////////////////////////////////

BOOL CFindDlg::Create(void) {

    BOOL bReturn = CDialog::Create (IDD_FIND_DLG);
//    GetDlgItem(IDC_PREV)->ShowWindow(SW_HIDE);
//    GetDlgItem(IDC_CASE_SENSITIVE)->ShowWindow(SW_HIDE);
    return bReturn;
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CFindDlg::EnableButtons
//
/////////////////////////////////////////////////////////////////////////////

void CFindDlg::EnableButtons (bool bEnableFlag)  {

    ((CButton*) GetDlgItem(IDC_NEXT))->EnableWindow(bEnableFlag);
    ((CButton*) GetDlgItem(IDC_PREV))->EnableWindow(bEnableFlag);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CFindDlg::UpdateHistoryList
//
/////////////////////////////////////////////////////////////////////////////

void CFindDlg::UpdateHistoryList(void) {

    CComboBox *pCB = (CComboBox *) GetDlgItem (IDC_SEARCH_TEXT);
    int i;

    pCB->ResetContent();
    if (m_csSearchText != m_csRecentFindSelection[0] && !m_csSearchText.IsEmpty()) {
        for (i = 14 ; i > 0 ; i--) {
            m_csRecentFindSelection[i] = m_csRecentFindSelection[i-1];
        }
        m_csRecentFindSelection[0] = m_csSearchText;
    }
    for (i = 14 ; i >= 0 ; i--)  {
        pCB->InsertString (0, m_csRecentFindSelection[i]);
    }
    pCB->SetCurSel (0);
}

/////////////////////////////////////////////////////////////////////////////
//
//                        CFindDlg::DataDataExchange
//
/////////////////////////////////////////////////////////////////////////////

void CFindDlg::DoDataExchange(CDataExchange* pDX) {

    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CFindDlg)
    DDX_Check(pDX, IDC_CASE_SENSITIVE, m_bCaseSensitive);
    DDX_CBString(pDX, IDC_SEARCH_TEXT, m_csSearchText);
    DDV_MaxChars(pDX, m_csSearchText, 100);
        // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CFindDlg::OnOK
//
/////////////////////////////////////////////////////////////////////////////


void CFindDlg::OnOK() {

    // Should not get here
    ASSERT (false);
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CFindDlg::OnCancel
//
/////////////////////////////////////////////////////////////////////////////


void CFindDlg::OnCancel() {

    // Need to override the default CDialog::OnCancel() because it calls EndDialog(),
    // which is a no-no in modeless dialog boxes.
    // In reality, when the user presses "Esc", he really meant to press "Close" ...
    OnClose ();
}


BEGIN_MESSAGE_MAP(CFindDlg, CDialog)
    //{{AFX_MSG_MAP(CFindDlg)
    ON_CBN_EDITCHANGE(IDC_SEARCH_TEXT, OnEditchangeSearchText)
    ON_CBN_SELCHANGE(IDC_SEARCH_TEXT, OnSelchangeSearchText)
    ON_BN_CLICKED(IDC_NEXT, OnNext)
    ON_BN_CLICKED(IDC_PREV, OnPrev)
    ON_BN_CLICKED(IDCLOSE, OnClose)
    // NOTE: the ClassWizard will add message map macros here
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
//
//                        CFindDlg::OnInitDialog
//
/////////////////////////////////////////////////////////////////////////////

BOOL CFindDlg::OnInitDialog() {

    CDialog::OnInitDialog();
    OnEditchangeSearchText();     // will gray next/prev buttons if there is no search text yet

    m_csSearchText.Empty();
    UpdateHistoryList ();
    if (!m_rcPos.IsRectEmpty()) {
        MoveWindow(m_rcPos);
    }
    else {
        CenterWindow();
    }
    return TRUE;                    // Return TRUE  unless you set the focus to a control
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CFindDlg::OnEditchangeSearchText
//
/////////////////////////////////////////////////////////////////////////////

void CFindDlg::OnEditchangeSearchText() {

    // This function is called every time a character is pressed in the "Find What" combo box
    CString csPrevSearchText = m_csSearchText;
    UpdateData (TRUE);
    EnableButtons (!m_csSearchText.IsEmpty());
    m_csSearchText = csPrevSearchText;
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CFindDlg::OnSelchangeSearchText
//
/////////////////////////////////////////////////////////////////////////////

void CFindDlg::OnSelchangeSearchText() {

    // This function is called when the user selects "Find What" text from the combo-box drop-down selections
    int iCurSel = ((CComboBox*) GetDlgItem (IDC_SEARCH_TEXT))->GetCurSel();
    CString csCurSel;
    ((CComboBox*) GetDlgItem (IDC_SEARCH_TEXT))->GetLBText(iCurSel, csCurSel);
    EnableButtons (!csCurSel.IsEmpty());
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CFindDlg::OnNext
//
/////////////////////////////////////////////////////////////////////////////

void CFindDlg::OnNext() {

    UpdateData (true);
    if (!m_csSearchText.IsEmpty() && m_csSearchText.GetLength() <= 100)  {   // Longest search string
        UpdateHistoryList();
        if (!m_bCaseSensitive)  {
            m_csSearchText.MakeUpper();
        }
        SetNext(true);
        ASSERT_VALID(m_pCurrView);
        m_pCurrView->PostMessage(UWM::Dictionary::Find);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CFindDlg::OnPrev
//
/////////////////////////////////////////////////////////////////////////////

void CFindDlg::OnPrev()  {

    UpdateData (true);
    if (!m_csSearchText.IsEmpty() && m_csSearchText.GetLength() <= 100) {   // Longest search string
        UpdateHistoryList();
        if (! m_bCaseSensitive)  {
            m_csSearchText.MakeUpper ();
        }
        SetNext(FALSE);
        ASSERT (m_pCurrView != NULL);
        m_pCurrView->PostMessage(UWM::Dictionary::Find);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                        CFindDlg::OnClose
//
/////////////////////////////////////////////////////////////////////////////

void CFindDlg::OnClose() {

    GetWindowRect(&m_rcPos);        // Remember dialog position
    CDictChildWnd* pWnd = (CDictChildWnd*) m_pCurrView->GetParentFrame();
    pWnd->SetFindActive(false);
    pWnd->InvalidateRect(m_rcPos);   // BMD 26 Apr 2002
    DestroyWindow ();
}
