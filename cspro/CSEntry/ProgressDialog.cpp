// ProgressDialog.cpp : implementation file
//

#include "StdAfx.h"
#include "ProgressDialog.h"
#include "afxdialogex.h"
#include <zUtilO/CustomFont.h>

// ProgressDialog dialog

namespace {
    const int PROGRESS_BAR_MAX = 10000;
}

IMPLEMENT_DYNAMIC(ProgressDialog, CDialog)

ProgressDialog::ProgressDialog()
    : CDialog(IDD_PROGRESS_DIALOG, NULL),
      m_isCancelled(false)
{

}

ProgressDialog::~ProgressDialog()
{
}

int ProgressDialog::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    return 0;
}

BOOL ProgressDialog::OnInitDialog()
{
    CDialog::OnInitDialog();

    // Progress bar is hidden initially - only shown when max progress is set
    CProgressCtrl* pProgressBar = (CProgressCtrl*)GetDlgItem(IDC_PROGRESS_BAR);
    pProgressBar->ShowWindow(SW_HIDE);
    // Progress messages use a fixed point number between zero and 10,000 for pct complete.
    pProgressBar->SetRange32(0, PROGRESS_BAR_MAX);

    SetUpFont();

    LayoutControls();

    return FALSE;
}

void ProgressDialog::SetMessageText(CString msg)
{
    m_messageText = msg;
    UpdateData(FALSE);
}

void ProgressDialog::SetUpFont()
{
    UserDefinedFonts* pUserFonts = nullptr;
    AfxGetMainWnd()->SendMessage(WM_IMSA_GET_USER_FONTS, (WPARAM)&pUserFonts);
    m_pFont = pUserFonts->GetFont(UserDefinedFonts::FontType::ErrMsg);

    if( m_pFont == nullptr )
    {
        // If no font specified use the same font as system controlled errmsg (MsgDial)
        m_defaultFont.CreateFont(17, 7, 0, 0, FW_NORMAL, FALSE, FALSE, 0, DEFAULT_CHARSET, OUT_STRING_PRECIS,
            CLIP_STROKE_PRECIS, PROOF_QUALITY, FF_DONTCARE, CString(_T("Courier New")));
        m_pFont = &m_defaultFont;
    }

    SetFont(m_pFont);
    SendMessageToDescendants(WM_SETFONT,
        (WPARAM)m_pFont->m_hObject,
        MAKELONG(FALSE, 0),
        FALSE);
}

void ProgressDialog::LayoutControls()
{
    // Calculate dimensions - 8 lines of text by roughly 70 characters but not to exceed 90%
    // of screen width

    CClientDC dc(this);
    dc.SelectObject(m_pFont);
    TEXTMETRIC metrics;
    dc.GetTextMetrics(&metrics);

    int characterWidth = metrics.tmAveCharWidth;
    int dialogWidth = characterWidth * 70;
    int maxWidth = (int)(GetSystemMetrics(SM_CXSCREEN) * 0.9);
    dialogWidth = std::min(dialogWidth, maxWidth);

    int textLineHeight = metrics.tmHeight;
    int captionHeight = GetSystemMetrics(SM_CYCAPTION);
    int dialogHeight = 8 * textLineHeight + captionHeight;
    SetWindowPos(NULL, 0, 0, dialogWidth, dialogHeight,
        SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE);

    CRect clientRect;
    GetClientRect(&clientRect);

    const int hMargin = characterWidth;
    const int vMargin = textLineHeight;

    CWnd* pMsgTxt = GetDlgItem(IDC_PROGRESS_MESSAGE);
    pMsgTxt->SetWindowPos(NULL,
        hMargin,
        clientRect.top + vMargin,
        clientRect.Width() - 2 * hMargin,
        textLineHeight,
        SWP_NOZORDER | SWP_NOACTIVATE);

    CProgressCtrl* pProgressBar = (CProgressCtrl*)GetDlgItem(IDC_PROGRESS_BAR);
    pProgressBar->SetWindowPos(NULL,
        hMargin,
        clientRect.top + 2 * vMargin + textLineHeight,
        clientRect.Width() - 2 * hMargin,
        textLineHeight,
        SWP_NOZORDER | SWP_NOACTIVATE);

    CButton* pCancelButton = (CButton*)GetDlgItem(IDCANCEL);
    CString cancelText;
    pCancelButton->GetWindowText(cancelText);
    CSize buttSize = dc.GetTextExtent(cancelText);
    buttSize.cx += 4 * characterWidth;
    buttSize.cy += textLineHeight / 2;
    pCancelButton->SetWindowPos(NULL,
        clientRect.Width() / 2 - buttSize.cx / 2,
        clientRect.bottom - buttSize.cy - vMargin,
        buttSize.cx, buttSize.cy,
        SWP_NOZORDER | SWP_NOACTIVATE);
}

void ProgressDialog::ShowModeless(CString text, CWnd* pParent)
{
    Create(IDD_PROGRESS_DIALOG, pParent);
    SetMessageText(text);
    ShowWindow(SW_SHOWNORMAL);
    CenterWindow();
    runMessageLoop(); // To process the init dialog msg immediately
}

void ProgressDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_PROGRESS_MESSAGE, m_messageText);
}


BEGIN_MESSAGE_MAP(ProgressDialog, CDialog)
    ON_BN_CLICKED(IDCANCEL, &ProgressDialog::OnBnClickedCancel)
    ON_WM_CREATE()
END_MESSAGE_MAP()

LRESULT ProgressDialog::Update(WPARAM wParam, LPARAM lParam)
{
    if (lParam) {
        SetMessageText(*((CString*)lParam));
    }

    CProgressCtrl* pProgressBar = (CProgressCtrl*)GetDlgItem(IDC_PROGRESS_BAR);
    if (int(wParam) == -1) {
        // Zero or less means indeterminate progress so don't show indicator
        pProgressBar->ShowWindow(SW_HIDE);
    }
    else if (int(wParam) >= 0) {
        pProgressBar->SetPos(wParam);
        pProgressBar->ShowWindow(SW_SHOW);
    }

    runMessageLoop();

    return m_isCancelled;
}

void ProgressDialog::runMessageLoop()
{
    MSG msg;
    while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }
}

void ProgressDialog::OnBnClickedCancel()
{
    SetMessageText("Canceling...");
    m_isCancelled = true;
}
