#pragma once

// CFmtFontDlg dialog
class CFmtFontDlg : public CFontDialog
{
    DECLARE_DYNAMIC(CFmtFontDlg)

public:
    CFmtFontDlg(LPLOGFONT lplfInitial = NULL,
        DWORD dwFlags = CF_EFFECTS | CF_SCREENFONTS,
        CDC* pdcPrinter = NULL,
        CWnd* pParentWnd = NULL);
#ifndef _AFX_NO_RICHEDIT_SUPPORT
    CFmtFontDlg(const CHARFORMAT& charformat,
        DWORD dwFlags = CF_SCREENFONTS,
        CDC* pdcPrinter = NULL,
        CWnd* pParentWnd = NULL);
#endif


    virtual ~CFmtFontDlg();

public:
    LOGFONT     m_lfDef;

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

    DECLARE_MESSAGE_MAP()
public:
    BOOL m_bUseDefault;
    BOOL m_bShowUseDefault;

    virtual INT_PTR DoModal();
    afx_msg void OnBnClickedUseDefFont();
};
