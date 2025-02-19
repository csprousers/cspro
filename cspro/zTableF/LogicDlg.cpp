#include "StdAfx.h"
#include "LogicDlg.h"


CEdtLogicDlg::CEdtLogicDlg(CWnd* pParent /*=NULL*/)
    :   CDialog(CEdtLogicDlg::IDD, pParent)
{
}


void CEdtLogicDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_EDT_LOGIC2, m_edtLogicCtrl);
}


BOOL CEdtLogicDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    //Create the custom logic control and subclass it.
    DWORD dwStyle = m_edtLogicCtrl.GetStyle();
    RECT rect;
    m_edtLogicCtrl.GetWindowRect(&rect);
    this->ScreenToClient(&rect);

    int nControlID = m_edtLogicCtrl.GetDlgCtrlID();
    m_edtLogicCtrl.DestroyWindow();


    //create the new window
    m_edtLogicCtrl.Create(dwStyle, rect, this, nControlID, WS_EX_CLIENTEDGE|WS_EX_LEFT|WS_EX_LTRREADING|WS_EX_RIGHTSCROLLBAR);
    m_edtLogicCtrl.InitLogicControl(false, false);


    if (m_bIsPostCalc) {    // BMD 05 Jun 2006
        SetWindowText(_T("Edit PostCalc Logic"));
    }
    else {
        SetWindowText(_T("Edit Tab Logic"));
    }

    //Initialise the edit control
    /*TODO:??? fontsize based on desinger zoom level???
    if(bRet){
        int fontSize = m_edtLogic.GetDefaultFontSize() * (GetDesignerFontZoomLevel() / 100.0);
        m_edtLogic.SetTextFont(0,fontSize);

    }*/
    UpdateData(FALSE);

    m_edtLogicCtrl.SetText(m_logic); //call base class instead of window text
    m_edtLogicCtrl.SetFocus();
    return TRUE;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}


BOOL CEdtLogicDlg::PreTranslateMessage(MSG* pMsg)
{
    // pass CTRL-T on main window to toggle names in dictionary tree
    if (pMsg->message == WM_KEYDOWN) {

        bool bCtrl = GetKeyState(VK_CONTROL) < 0;
        bool bT = (pMsg->wParam == _T('t') || pMsg->wParam == _T('T'));

        if (bCtrl && bT) {
            CFrameWnd* pMainWnd = (CFrameWnd*) GetParent();
            pMainWnd->SendMessage(WM_COMMAND, ID_VIEW_NAMES);
            return TRUE;
        }
    }

    return CDialog::PreTranslateMessage(pMsg);
}


void CEdtLogicDlg::OnOK()
{
    m_logic = m_edtLogicCtrl.GetText();

    CDialog::OnOK();
}
