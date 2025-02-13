#include "StdAfx.h"
#include "ProgressDlg2.h"


ProgressDlg2::ProgressDlg2(CWnd* pParent /*=NULL*/)
    :   CDialog(IDD_PROGRESS2, pParent),
        m_bCancel(false),
        m_bParentDisabled(false)      
{
}

ProgressDlg2::~ProgressDlg2()
{
    if (m_hWnd != NULL) {
        DestroyWindow();
    }
}


void ProgressDlg2::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(ProgressDlg2)
    DDX_Control(pDX, IDC_PROGRESS_OVERALL, m_progressOverall);
    DDX_Control(pDX, IDC_PROGRESS_CURRENT, m_progressCurrent);
    //}}AFX_DATA_MAP
}


BOOL ProgressDlg2::Create(CWnd* pParentWnd)
{
    // Get the true parent of the dialog
    m_pParentWnd = CWnd::GetSafeOwner(pParentWnd);

    // m_bParentDisabled is used to re-enable the parent window
    // when the dialog is destroyed. So we don't want to set
    // it to TRUE unless the parent was already enabled.

    if((m_pParentWnd != NULL) && m_pParentWnd->IsWindowEnabled()) {
        m_pParentWnd->EnableWindow(FALSE);
        m_bParentDisabled = true;
    }

    if(!CDialog::Create(IDD_PROGRESS2, pParentWnd)) {
        ReEnableParent();
        return FALSE;
    }
    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////////
//              ProgressDlg2::HideOverallIndicators
// turn off the overall progress controls (show only current)
/////////////////////////////////////////////////////////////////////////////////
void ProgressDlg2::HideOverallIndicators()
{
    ASSERT(m_hWnd != NULL);

    CRect currentBoxRect;
    CWnd* pCurrGroupBox = GetDlgItem(IDC_GROUP_BOX_CURRENT);
    ASSERT(pCurrGroupBox);
    pCurrGroupBox->GetWindowRect(&currentBoxRect);
    ScreenToClient(&currentBoxRect);

    // hide the items for overall
    pCurrGroupBox->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_GROUP_BOX_OVERALL)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_STATUS_OVERALL)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_OVERALL_PERCENT)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_PROGRESS_OVERALL)->ShowWindow(SW_HIDE);

    // move cancel button up
    CWnd* pCancel = GetDlgItem(IDCANCEL);
    ASSERT(pCancel);
    CRect cancelRect;
    pCancel->GetWindowRect(&cancelRect);
    ScreenToClient(&cancelRect);
    pCancel->MoveWindow(cancelRect.left, currentBoxRect.bottom,
                        cancelRect.Width(), cancelRect.Height());

    // move the bottom of this window up
    CRect currDlgRect;
    GetWindowRect(&currDlgRect);
    currDlgRect.bottom -= cancelRect.bottom - currentBoxRect.bottom - cancelRect.Height();
    MoveWindow(&currDlgRect);
}

void ProgressDlg2::ReEnableParent()
{
    if(m_bParentDisabled && (m_pParentWnd!=NULL)) {
        m_pParentWnd->EnableWindow(TRUE);
    }
    m_bParentDisabled = false;
}

BOOL ProgressDlg2::DestroyWindow()
{
    ReEnableParent();
    return CDialog::DestroyWindow();
}

BOOL ProgressDlg2::OnInitDialog()
{
    BOOL bResult = CDialog::OnInitDialog();

    m_controls[CURRENT].SetControls(GetDlgItem(IDC_STATUS_CURRENT),
                                  GetDlgItem(IDC_GROUP_BOX_CURRENT),
                                  GetDlgItem(IDC_CURRENT_PERCENT),
                                  &m_progressCurrent);
    m_controls[OVERALL].SetControls(GetDlgItem(IDC_STATUS_OVERALL),
                                  GetDlgItem(IDC_GROUP_BOX_OVERALL),
                                  GetDlgItem(IDC_OVERALL_PERCENT),
                                  &m_progressOverall);

    return bResult;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void ProgressDlg2::OnCancel()
{
    m_bCancel = true;
}

/////////////////////////////////////////////////////////////////////////////
//
//                         ProgressDlg2::CheckCancelButton
//
/////////////////////////////////////////////////////////////////////////////

BOOL ProgressDlg2::CheckCancelButton() {

    // Process all pending messages
    PumpMessages();

    // Reset m_bCancel to FALSE so that
    // CheckCancelButton returns FALSE until the user
    // clicks Cancel again. This will allow you to call
    // CheckCancelButton and still continue the operation.
    // If m_bCancel stayed TRUE, then the next call to
    // CheckCancelButton would always return TRUE

    BOOL bResult = m_bCancel;
    m_bCancel = FALSE;

    return bResult;
}

/////////////////////////////////////////////////////////////////////////////
//
//                         ProgressDlg2::SetStatus
//
/////////////////////////////////////////////////////////////////////////////

void ProgressDlg2::SetStatus(WhichProgressControls w, LPCTSTR lpszMessage)
{
    m_controls[w].SetStatus(lpszMessage);
}

/////////////////////////////////////////////////////////////////////////////
//
//                         ProgressDlg2::SetGroupBoxText
//
/////////////////////////////////////////////////////////////////////////////

void ProgressDlg2::SetGroupBoxText(WhichProgressControls w, LPCTSTR lpszMessage)
{
    m_controls[w].SetGroupBoxText(lpszMessage);
}

/////////////////////////////////////////////////////////////////////////////
//
//                         ProgressDlg2::SetRange
//
/////////////////////////////////////////////////////////////////////////////

void ProgressDlg2::SetRange(WhichProgressControls w, int iLower,int iUpper)
{
    m_controls[w].SetRange(iLower, iUpper);
}

/////////////////////////////////////////////////////////////////////////////
//
//                         ProgressDlg2::GetUpperRange
//
/////////////////////////////////////////////////////////////////////////////

int ProgressDlg2::GetUpperRange(WhichProgressControls w)
{
    return m_controls[w].GetUpperRange();
}

/////////////////////////////////////////////////////////////////////////////
//
//                         ProgressDlg2::SetStep
//
/////////////////////////////////////////////////////////////////////////////

int ProgressDlg2::SetStep(WhichProgressControls w, int iStep)
{
    return m_controls[w].SetStep(iStep);
}

/////////////////////////////////////////////////////////////////////////////
//
//                         ProgressDlg2::SetPos
//
/////////////////////////////////////////////////////////////////////////////
int ProgressDlg2::SetPos(WhichProgressControls w, int iPos)
{
    PumpMessages();
    return m_controls[w].SetPos(iPos);
}

/////////////////////////////////////////////////////////////////////////////
//
//                         ProgressDlg2::OffsetPos
//
/////////////////////////////////////////////////////////////////////////////
int ProgressDlg2::OffsetPos(WhichProgressControls w, int iPos)
{
    PumpMessages();
    return m_controls[w].OffsetPos(iPos);
}

/////////////////////////////////////////////////////////////////////////////
//
//                         ProgressDlg2::StepIt
//
/////////////////////////////////////////////////////////////////////////////
int ProgressDlg2::StepIt(WhichProgressControls w)
{
    PumpMessages();
    return m_controls[w].StepIt();
}

/////////////////////////////////////////////////////////////////////////////
//
//                         ProgressDlg2::PumpMessages
//
/////////////////////////////////////////////////////////////////////////////

void ProgressDlg2::PumpMessages() {

    // Must call Create() before using the dialog
    ASSERT(m_hWnd!=NULL);

    MSG msg;
    // Handle dialog messages
    while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (!IsDialogMessage(&msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                         ProgressDlg2::ProgressControls::SetStatus
//
/////////////////////////////////////////////////////////////////////////////
void ProgressDlg2::ProgressControls::SetStatus(LPCTSTR lpszMessage)
{
    m_pWndStatus->SetWindowText( lpszMessage );
}

/////////////////////////////////////////////////////////////////////////////
//
//                         ProgressDlg2::ProgressControls::SetGroupBoxText
//
/////////////////////////////////////////////////////////////////////////////
void ProgressDlg2::ProgressControls::SetGroupBoxText(LPCTSTR lpszMessage)
{
    m_pWndGroupBox->SetWindowText(lpszMessage);
}

///////////////////////////////////////////////////////////////////////////
//
//                         ProgressDlg2::ProgressControls::SetRange
//
/////////////////////////////////////////////////////////////////////////////
void ProgressDlg2::ProgressControls::SetRange(int iLower,int iUpper)
{
    ASSERT(iLower < iUpper);

    m_iLower = iLower;
    m_iUpper = iUpper;
    m_pProgressCtrl->SetRange32(iLower, iUpper);
}

///////////////////////////////////////////////////////////////////////////
//
//                         ProgressDlg2::ProgressControls::GetUpperRange
//
/////////////////////////////////////////////////////////////////////////////
int ProgressDlg2::ProgressControls::GetUpperRange()
{
    return m_iUpper;
}

/////////////////////////////////////////////////////////////////////////////
//
//                         ProgressDlg2::ProgressControls::SetPos
//
/////////////////////////////////////////////////////////////////////////////
int ProgressDlg2::ProgressControls::SetPos(int iPos)
{
    int iResult = m_pProgressCtrl->SetPos(iPos);
    UpdatePercent(iPos);
    return iResult;
}


/////////////////////////////////////////////////////////////////////////////
//
//                         ProgressDlg2::ProgressControls::SetStep
//
/////////////////////////////////////////////////////////////////////////////
int ProgressDlg2::ProgressControls::SetStep(int iStep)
{
    m_iStep = iStep;
    return m_pProgressCtrl->SetStep(iStep);
}


/////////////////////////////////////////////////////////////////////////////
//
//                         ProgressDlg2::ProgressControls::OffsetPos
//
/////////////////////////////////////////////////////////////////////////////
int ProgressDlg2::ProgressControls::OffsetPos(int iPos)
{
    int iResult = m_pProgressCtrl->OffsetPos(iPos);
    UpdatePercent(iResult + iPos);
    return iResult;
}


/////////////////////////////////////////////////////////////////////////////
//
//                         ProgressDlg2::ProgressControls::StepIt
//
/////////////////////////////////////////////////////////////////////////////
int ProgressDlg2::ProgressControls::StepIt()
{
    int iResult = m_pProgressCtrl->StepIt();
    UpdatePercent(iResult + m_iStep);
    return iResult;
}

/////////////////////////////////////////////////////////////////////////////
//
//                         ProgressDlg2::ProgressControls::UpdatePercent
//
/////////////////////////////////////////////////////////////////////////////
void ProgressDlg2::ProgressControls::UpdatePercent(int iNewPos)
{
    int iPercent;

    int iDivisor = m_iUpper - m_iLower;
    if (iDivisor <= 0)  {
        iDivisor = 1;
    }

    int iDividend = (iNewPos - m_iLower);
    ASSERT(iDividend >= 0);   // Current position should be greater than m_iLower

    iPercent = iDividend * 100 / iDivisor;

    // Since the Progress Control wraps, we will wrap the percentage
    // along with it. However, don't reset 100% back to 0%
    if (iPercent != 100) {
        iPercent %= 100;
    }

    // Display the percentage
    CString strBuf;
    strBuf.Format(_T("%d%lc"),iPercent,_T('%'));

    CString strCur; // get current percentage
    m_pWndPercent->GetWindowText(strCur);

    if (strCur != strBuf) {
        m_pWndPercent->SetWindowText(strBuf);
    }
}
