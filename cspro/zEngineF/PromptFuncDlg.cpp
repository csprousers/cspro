#include "StdAfx.h"
#include "PromptFuncDlg.h"
#include <zUtilO/CustomFont.h>


// CPromptFunctionDlg dialog

IMPLEMENT_DYNAMIC(CPromptFunctionDlg, CDialogEx)

BEGIN_MESSAGE_MAP(CPromptFunctionDlg, CDialogEx)
END_MESSAGE_MAP()


CPromptFunctionDlg::CPromptFunctionDlg(const CString& csTitle,const CString& csInitialResponse,bool bMultiline,bool bNumeric,bool bPassword,bool bUpperCase)
    :   CDialogEx(IDD_PROMPT_FUNC),
        m_csTitle(csTitle),
        m_csInitialResponse(csInitialResponse),
        m_bMultiline(bMultiline),
        m_bNumeric(bNumeric),
        m_bPassword(bPassword),
        m_bUpperCase(bUpperCase)
{
}


CPromptFunctionDlg::~CPromptFunctionDlg()
{
}


void CPromptFunctionDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}


BOOL CPromptFunctionDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    CFont* pFont = nullptr;

    UserDefinedFonts* pUserFonts = nullptr;
    AfxGetApp()->GetMainWnd()->SendMessage(WM_IMSA_GET_USER_FONTS, (WPARAM)&pUserFonts);

    if( pUserFonts != nullptr )
        pFont = pUserFonts->GetFont(UserDefinedFonts::FontType::ErrMsg);

    if( pFont == nullptr )
    {
        m_Font.CreateFont(18,0,0,0,400,FALSE,FALSE,0,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,FF_DONTCARE,_T("Arial"));
        pFont = &m_Font;
    }

    CClientDC dc(this);
    dc.SelectObject(pFont);

    // try to get a good width for this dialog; we'll try to keep it at fixed width so that the dialog's size doesn't wildly
    // vary due to different titles, but we'll also want to increase the box's size if necesseary
    const double MaxWidthProportion = 0.90;
    const int DesiredWidth = 500;

    int iMaxWidth = (int)( GetSystemMetrics(SM_CXSCREEN) * MaxWidthProportion );
    int iIdealWidth = std::min(iMaxWidth,DesiredWidth);

    CRect sizingRect;
    ZeroMemory(&sizingRect,sizeof(CRect));
    sizingRect.right = iIdealWidth;
    dc.DrawText(m_csTitle,&sizingRect,DT_CALCRECT | DT_WORDBREAK);

    // if the height is quite high, then increase the width of the box
    if( sizingRect.Height() > iIdealWidth )
    {
        iIdealWidth = std::min(iMaxWidth, ( DesiredWidth + iMaxWidth ) / 2);
        sizingRect.right = iIdealWidth;
        dc.DrawText(m_csTitle,&sizingRect,DT_CALCRECT | DT_WORDBREAK);
    }

    CRect rect;
    CStatic* pTitle = (CStatic*)GetDlgItem(IDC_PROMPT_TITLE);
    pTitle->GetWindowRect(&rect);

    int iWidthIncrease = std::max(0,sizingRect.Width() - rect.Width());
    int iHeightIncrease = std::max(0,sizingRect.Height() - rect.Height());

    // do the adjustments
    ScreenToClient(&rect);
    pTitle->SetWindowPos(NULL,0,0,rect.Width() + iWidthIncrease,rect.Height() + iHeightIncrease,SWP_NOMOVE | SWP_NOZORDER);

    CEdit* pResponseSingleLineEdit = (CEdit*)GetDlgItem(IDC_PROMPT_EDIT_SINGLELINE);
    CEdit* pResponseMultilineEdit = (CEdit*)GetDlgItem(IDC_PROMPT_EDIT_MULTILINE);

    if( m_bMultiline )
    {
        const int MultilineMultiplier = 3;

        pResponseMultilineEdit->GetWindowRect(&rect);
        ScreenToClient(&rect);

        int iMultilineHeightIncrease = rect.Height() * MultilineMultiplier;

        pResponseMultilineEdit->SetWindowPos(NULL,rect.left,rect.top + iHeightIncrease,rect.Width() + iWidthIncrease,rect.Height() + iMultilineHeightIncrease,SWP_NOZORDER);
        iHeightIncrease += iMultilineHeightIncrease;

        m_pResponseEdit = pResponseMultilineEdit;
        pResponseSingleLineEdit->ShowWindow(SW_HIDE);
    }

    else
    {
        pResponseSingleLineEdit->GetWindowRect(&rect);
        ScreenToClient(&rect);
        pResponseSingleLineEdit->SetWindowPos(NULL,rect.left,rect.top + iHeightIncrease,rect.Width() + iWidthIncrease,rect.Height(),SWP_NOZORDER);

        m_pResponseEdit = pResponseSingleLineEdit;
        pResponseMultilineEdit->ShowWindow(SW_HIDE);
    }

    CButton* pButton = (CButton*)GetDlgItem(IDOK);
    pButton->GetWindowRect(&rect);
    ScreenToClient(&rect);
    pButton->SetWindowPos(NULL,rect.left + iWidthIncrease / 2,rect.top + iHeightIncrease,0,0,SWP_NOSIZE | SWP_NOZORDER);

    pButton = (CButton*)GetDlgItem(IDCANCEL);
    pButton->GetWindowRect(&rect);
    ScreenToClient(&rect);
    pButton->SetWindowPos(NULL,rect.left + iWidthIncrease / 2,rect.top + iHeightIncrease,0,0,SWP_NOSIZE | SWP_NOZORDER);

    this->GetWindowRect(&rect);
    this->SetWindowPos(NULL,rect.left,rect.top,rect.Width() + iWidthIncrease,rect.Height() + iHeightIncrease,SWP_NOZORDER);

    // set the styles
    if( m_bNumeric )
        m_pResponseEdit->ModifyStyle(0,ES_NUMBER);

    if( m_bPassword )
        m_pResponseEdit->SetPasswordChar(_T('*'));

    if( m_bUpperCase )
        m_pResponseEdit->ModifyStyle(0,ES_UPPERCASE);

    // set the initial values
    pTitle->SetFont(pFont);
    pTitle->SetWindowText(m_csTitle);

    m_pResponseEdit->SetWindowText(m_csInitialResponse);
    m_pResponseEdit->SetSel(m_csInitialResponse.GetLength(),m_csInitialResponse.GetLength());
    m_pResponseEdit->SetFocus();

    return FALSE;
}


void CPromptFunctionDlg::OnOK()
{
    m_pResponseEdit->GetWindowText(m_csResponse);
    CDialog::OnOK();
}
