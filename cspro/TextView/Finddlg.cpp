//***************************************************************************
//  File name: FindDlg.cpp
//
//  Description:
//       Implementation of CFindDlg class
//
//  History:    Date       Author     Comment
//              -----------------------------
//              1995        csc       created
//              28 feb 97   csc       OnSelchangeSearchText changed
//              4 jan 03    csc       added kick idle handler, fixed button update bug
//              4 jan 03    csc       fixed bug in case sensitivity
//              4 jan 03    csc       made find history persistent
//              24 Jul 07   jh        got rid of all the kick idle stuff that wasn't working and replaced with combo box notification
//
//***************************************************************************

#include "StdAfx.h"
#include <zUtilO/Interapp.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define HID_BASE_RESOURCE 0x00020000UL

/////////////////////////////////////////////////////////////////////////////
// CFindDlg dialog


CFindDlg::CFindDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CFindDlg::IDD, pParent)  {
    //{{AFX_DATA_INIT(CFindDlg)
    m_bCaseSensitive = FALSE;
    m_csSearchText = _T("");
    //}}AFX_DATA_INIT
}

CFindDlg::~CFindDlg ()  {
    // csc 4 jan 03
    CString cs;
    for (int i=0 ; i<HISTORY_SIZE ; i++)  {
        cs.Format(_T("Find%d"), i);
        AfxGetApp()->WriteProfileString(_T("Recent Find List"), cs, m_csRecentFindSelection[i]);
    }
}

BOOL CFindDlg::Create ()  {
    return CDialog::Create (CFindDlg::IDD);
}

void CFindDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CFindDlg)
    DDX_Check(pDX, IDC_CASE_SENSITIVE, m_bCaseSensitive);
    DDX_CBString(pDX, IDC_SEARCH_TEXT, m_csSearchText);
    DDV_MaxChars(pDX, m_csSearchText, 100);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CFindDlg, CDialog)
    //{{AFX_MSG_MAP(CFindDlg)
    ON_BN_CLICKED(ID_NEXT, OnNext)
    ON_BN_CLICKED(ID_PREV, OnPrev)
    ON_BN_CLICKED(IDCLOSE, OnClose)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CFindDlg message handlers

BOOL CFindDlg::OnInitDialog()
{
    static BOOL bFirstTime = TRUE;

    CDialog::OnInitDialog();
//    OnEditchangeSearchText();     // will gray next/prev buttons if there is no search text yet
//    m_csSearchText.Empty();

    // csc 4 jan 03
    if (bFirstTime)  {
        CString cs;
        for (int i=0 ; i<HISTORY_SIZE ; i++)  {
            cs.Format(_T("Find%d"), i);
            m_csRecentFindSelection[i] = AfxGetApp()->GetProfileString(_T("Recent Find List"), cs);
        }
        bFirstTime = FALSE;
    }
    UpdateHistoryList ();

    // CG: The following block was added by the ToolTips component.
    {
        // Create the ToolTip control.
        m_tooltip.Create(this);
        m_tooltip.Activate(TRUE);

        // TODO: Use one of the following forms to add controls:
        // m_tooltip.AddTool(GetDlgItem(IDC_<name>), <string-table-id>);
        // m_tooltip.AddTool(GetDlgItem(IDC_<name>), "<text>");
    }

    return TRUE;  // return TRUE  unless you set the focus to a control
}

void CFindDlg::UpdateHistoryList (void)  {
    CComboBox *pCB = (CComboBox *) GetDlgItem (IDC_SEARCH_TEXT);
    int i;

    pCB->ResetContent();
    if ( m_csSearchText != m_csRecentFindSelection[0] && ! m_csSearchText.IsEmpty() )  {
        for (i = HISTORY_SIZE-1 ; i > 0 ; i-- )  {
            m_csRecentFindSelection[i] = m_csRecentFindSelection[i-1];
            //pCB->InsertString (0, m_csRecentFindSelection[i]);
        }
        m_csRecentFindSelection[0] = m_csSearchText;
    }
    for (i = HISTORY_SIZE-1 ; i >= 0 ; i-- )  {
        pCB->InsertString (0, m_csRecentFindSelection[i]);
    }
    pCB->SetCurSel (0);
}

void CFindDlg::OnNext()  {
    // find next occurrence of specified text...
    UpdateData (TRUE);
    if (! m_csSearchText.IsEmpty() && m_csSearchText.GetLength() <= 100)  {   // 100 characters is the longest search string
        UpdateHistoryList();
        if (! m_bCaseSensitive)  {
            // make the search key upper case ...
            m_csSearchText.MakeUpper();
        }
        ASSERT (m_pCurrView != NULL);
        SetDirection (SEARCH_FORWARD);
        m_pCurrView->PostMessage(UWM::TextView::Search);
    }
}

void CFindDlg::OnPrev()  {
    // find previous occurrence of specified text...
    UpdateData (TRUE);
    if (! m_csSearchText.IsEmpty() && m_csSearchText.GetLength() <= 100)  {   // 100 characters is the longest search string
        UpdateHistoryList();
        if (! m_bCaseSensitive)  {
            // make the search key upper case ...
            m_csSearchText.MakeUpper ();
        }
        ASSERT (m_pCurrView != NULL);
        SetDirection(SEARCH_BACKWARD);
        m_pCurrView->PostMessage(UWM::TextView::Search);
    }
}

void CFindDlg::OnClose()  {
    ASSERT ( m_pCurrView != NULL );
    UpdateData(TRUE);
    m_pCurrView->PostMessage(UWM::TextView::SearchClose, NULL);
    DestroyWindow ();
}

void CFindDlg::OnOK ()  {
    ASSERT (FALSE);
}

void CFindDlg::OnCancel ()  {
    // need to override the default CDialog::OnCancel() because it calls EndDialog(), which is a no-no in
    // modeless dialog boxes.  In reality, when the user presses "Esc", he really meant to press "Close" ...
    OnClose ();
}


BOOL CFindDlg::PreTranslateMessage(MSG* pMsg)
{
    if(IsDialogMessage(pMsg))  {
        if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_F3 && (::GetKeyState(VK_CONTROL) & 0x8000)) {
            SendMessage(WM_COMMAND, ID_PREV);
        }
        else if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_F3) {
            SendMessage(WM_COMMAND, ID_NEXT);
        }
            return TRUE;
    }

    m_tooltip.RelayEvent(pMsg);
    return CDialog::PreTranslateMessage(pMsg);
}
