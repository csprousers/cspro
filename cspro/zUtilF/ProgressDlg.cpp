#include "StdAfx.h"
#include "ProgressDlg.h"


/////////////////////////////////////////////////////////////////////////////
//
//                         ProgressDlg::ProgressDlg
//
/////////////////////////////////////////////////////////////////////////////

ProgressDlg::ProgressDlg()
{
    m_iLower = 0;
    m_iUpper = 100;
    m_iStep = 1;
    m_bCancel = FALSE;
    m_bParentDisabled = false;
}


/////////////////////////////////////////////////////////////////////////////
//
//                         ProgressDlg::~ProgressDlg
//
/////////////////////////////////////////////////////////////////////////////

ProgressDlg::~ProgressDlg()
{
    if (m_hWnd != NULL) {
        DestroyWindow();
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                         ProgressDlg::Create
//
/////////////////////////////////////////////////////////////////////////////

BOOL ProgressDlg::Create(CWnd *pParent)
{
    // Get the true parent of the dialog
    m_pParentWnd = CWnd::GetSafeOwner(pParent);

    // Don't use hidden window as parent - suspect that it causes hang when generating pen file
    if(m_pParentWnd != nullptr && !m_pParentWnd->IsWindowVisible())
        m_pParentWnd = nullptr;

    // m_bParentDisabled is used to re-enable the parent window
    // when the dialog is destroyed. So we don't want to set
    // it to TRUE unless the parent was already enabled.
    if((m_pParentWnd != NULL) && m_pParentWnd->IsWindowEnabled()) {
        m_pParentWnd->EnableWindow(FALSE);
        m_bParentDisabled = true;
    }

    if(!CDialog::Create(IDD_PROGRESS, pParent)) {
        ReEnableParent();
        return FALSE;
    }
    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
//
//                         ProgressDlg::DestroyWindow
//
/////////////////////////////////////////////////////////////////////////////
BOOL ProgressDlg::DestroyWindow()
{
    ReEnableParent();
    return CDialog::DestroyWindow();
}

/////////////////////////////////////////////////////////////////////////////
//
//                         ProgressDlg::ReEnableParent
//
/////////////////////////////////////////////////////////////////////////////

void ProgressDlg::ReEnableParent()
{
    if(m_bParentDisabled && (m_pParentWnd!=NULL)) {
        m_pParentWnd->EnableWindow(TRUE);
    }
    m_bParentDisabled = false;
}


/////////////////////////////////////////////////////////////////////////////
//
//                         ProgressDlg::DoDataExchange
//
/////////////////////////////////////////////////////////////////////////////
void ProgressDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(ProgressDlg)
    DDX_Control(pDX, IDC_PROGDLG_PROGRESS, m_progress);
    //}}AFX_DATA_MAP
}


void ProgressDlg::SetCaption(UINT caption_id)
{
    CString strCaption;
    VERIFY(strCaption.LoadString(caption_id));
    SetWindowText(strCaption);
}

/////////////////////////////////////////////////////////////////////////////
//
//                         ProgressDlg::SetStatus
//
/////////////////////////////////////////////////////////////////////////////

void ProgressDlg::SetStatus(LPCTSTR lpszMessage)
{
    ASSERT(m_hWnd); // Don't call this _before_ the dialog has
                    // been created. Can be called from OnInitDialog
    CWnd *pWndStatus = GetDlgItem(IDC_PROGDLG_STATUS);
    ASSERT(pWndStatus != NULL);

    if(!pWndStatus)
        return;

    if(!IsWindow(pWndStatus->GetSafeHwnd()))
        return;

    pWndStatus->SetWindowText(lpszMessage);
}


/////////////////////////////////////////////////////////////////////////////
//
//                         ProgressDlg::SetStatus
//
/////////////////////////////////////////////////////////////////////////////

void ProgressDlg::SetStatus(UINT uID)
{
    CString cs;
    cs.LoadString(uID);
    SetStatus(cs);
}


/////////////////////////////////////////////////////////////////////////////
//
//                         ProgressDlg::OnCancel
//
/////////////////////////////////////////////////////////////////////////////

void ProgressDlg::OnCancel()
{
    m_bCancel = TRUE;
}


/////////////////////////////////////////////////////////////////////////////
//
//                         ProgressDlg::SetRange
//
/////////////////////////////////////////////////////////////////////////////

void ProgressDlg::SetRange(int iLower, int iUpper)
{
    ASSERT(iLower < iUpper);

    m_iLower = iLower;
    m_iUpper = iUpper;

    if(IsWindow(m_progress.GetSafeHwnd()))
        m_progress.SetRange((short) iLower, (short) iUpper);
}


/////////////////////////////////////////////////////////////////////////////
//
//                         ProgressDlg::SetPos
//
/////////////////////////////////////////////////////////////////////////////

int ProgressDlg::SetPos(int iPos)
{
    PumpMessages();
    int iResult = m_progress.SetPos(iPos);
    UpdatePercent(iPos);
    return iResult;
}


/////////////////////////////////////////////////////////////////////////////
//
//                         ProgressDlg::SetStep
//
/////////////////////////////////////////////////////////////////////////////

int ProgressDlg::SetStep(int iStep)
{
    m_iStep = iStep;
    return m_progress.SetStep(iStep);
}


/////////////////////////////////////////////////////////////////////////////
//
//                         ProgressDlg::OffsetPos
//
/////////////////////////////////////////////////////////////////////////////

int ProgressDlg::OffsetPos(int iPos)
{
    PumpMessages();
    int iResult = m_progress.OffsetPos(iPos);
    UpdatePercent(iResult + iPos);
    return iResult;
}


/////////////////////////////////////////////////////////////////////////////
//
//                         ProgressDlg::StepIt
//
/////////////////////////////////////////////////////////////////////////////

int ProgressDlg::StepIt()
{
    PumpMessages();
    int iResult = m_progress.StepIt();
    UpdatePercent(iResult + m_iStep);
    return iResult;
}


/////////////////////////////////////////////////////////////////////////////
//
//                         ProgressDlg::ProgressDlg
//
/////////////////////////////////////////////////////////////////////////////

void ProgressDlg::PumpMessages()
{
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
//                         ProgressDlg::ProgressDlg
//
/////////////////////////////////////////////////////////////////////////////

BOOL ProgressDlg::CheckCancelButton()
{
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
//                         ProgressDlg::UpdatePercent
//
/////////////////////////////////////////////////////////////////////////////

void ProgressDlg::UpdatePercent(int iNewPos)
{
    CWnd *pWndPercent = GetDlgItem(IDC_PROGDLG_PERCENT);
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

    pWndPercent->GetWindowText(strCur);

    if (strCur != strBuf) {
        pWndPercent->SetWindowText(strBuf);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                         ProgressDlg::OnInitDialog
//
/////////////////////////////////////////////////////////////////////////////
BOOL ProgressDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    m_progress.SetRange((short)m_iLower, (short)m_iUpper);
    m_progress.SetStep(m_iStep);
    m_progress.SetPos(m_iLower);

    SetWindowText(CSPRO_VERSION);

    return TRUE;
}
