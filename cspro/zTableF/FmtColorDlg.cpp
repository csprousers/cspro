// FmtClDlg.cpp : implementation file
//

#include "StdAfx.h"
#include "FmtClDlg.h"

UINT iRGBMSG;
bool bDefColor;
bool bShowUseDefColor;

UINT_PTR CALLBACK CCHookProc(HWND hdlg, UINT uiMsg,WPARAM wParam,LPARAM lParam)
{
    if(wParam == IDC_DEF_CLR ){
        SendMessage(hdlg, iRGBMSG, 0,lParam);
    }
    if(uiMsg == WM_INITDIALOG){
        HWND hWnd = GetDlgItem(hdlg,IDC_DEF_CLR);
        if(IsWindow(hWnd)){
            CButton button;
            button.Attach(hWnd);
            bDefColor ? button.SetCheck(TRUE) : button.SetCheck(FALSE);
            button.Detach();
            /*if(bDefColor){
                HWND hWnd = GetDlgItem(hdlg,720);
                if(IsWindow(hWnd)){
                    CWnd wnd;
                    wnd.Attach(hWnd);
                    wnd.EnableWindow(FALSE);
                    wnd.Detach();
                }
            }*/
            if (!bShowUseDefColor) {
                ShowWindow(hWnd, SW_HIDE);
            }
        }
    }
    return 0;
}
// CFmtColorDlg dialog

IMPLEMENT_DYNAMIC(CFmtColorDlg, CColorDialog)
CFmtColorDlg::CFmtColorDlg(COLORREF clrInit /*= 0*/,DWORD dwFlags /*= 0*/,CWnd* pParentWnd /*= NULL*/)
    : CColorDialog(clrInit,dwFlags,pParentWnd)
    , m_bUseDef(FALSE)
    , m_bShowUseDefault(TRUE)
{
    m_colorDef = RGB(0,0,0);
}

CFmtColorDlg::~CFmtColorDlg()
{
}

void CFmtColorDlg::DoDataExchange(CDataExchange* pDX)
{
    CColorDialog::DoDataExchange(pDX);
    DDX_Check(pDX, IDC_DEF_CLR, m_bUseDef);
}


BEGIN_MESSAGE_MAP(CFmtColorDlg, CColorDialog)
    ON_BN_CLICKED(IDC_DEF_CLR, OnBnClickedDefClr)
END_MESSAGE_MAP()


// CFmtColorDlg message handlers

BOOL CFmtColorDlg::OnInitDialog()
{
    CColorDialog::OnInitDialog();

    // TODO:  Add extra initialization here

    return TRUE;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}

void CFmtColorDlg::OnBnClickedDefClr()
{
    if(((CButton*)GetDlgItem(IDC_DEF_CLR))->GetCheck() == BST_CHECKED){
        //Disable other buttons ;
         m_bUseDef=TRUE;
         m_cc.lpfnHook(GetSafeHwnd(),iRGBMSG,(WPARAM)IDC_DEF_CLR,(LPARAM)m_colorDef);
    }
    else { //Enable other buttons
        m_bUseDef = FALSE;
      //  m_cc.lpfnHook(GetSafeHwnd(),iRGBMSG,(WPARAM)IDC_DEF_CLR,(LPARAM)0);
    }
}

INT_PTR CFmtColorDlg::DoModal()
{
    iRGBMSG = RegisterWindowMessage(SETRGBSTRING);
    !m_bUseDef ? bDefColor = false : bDefColor = true;
    !m_bShowUseDefault ? bShowUseDefColor = false : bShowUseDefColor = true;
    // TODO: Add your specialized code here and/or call the base class
    m_cc.Flags |= CC_ENABLETEMPLATE|CC_ENABLEHOOK;
    m_cc.hInstance = (HWND)::GetModuleHandle(_T("zTableF.dll"));
    m_cc.lpTemplateName = MAKEINTRESOURCE(IDD_FMT_COLORDLG);
    m_cc.lpfnHook = CCHookProc;

    return CColorDialog::DoModal();
}

void CFmtColorDlg::OnOK()
{
    // TODO: Add your specialized code here and/or call the base class

    CColorDialog::OnOK();
}
