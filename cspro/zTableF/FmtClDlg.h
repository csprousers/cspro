#pragma once

// CFmtColorDlg dialog

class CFmtColorDlg : public CColorDialog
{
    DECLARE_DYNAMIC(CFmtColorDlg)

public:
    CFmtColorDlg(COLORREF clrInit = 0,DWORD dwFlags = 0,CWnd* pParentWnd = NULL);
    virtual ~CFmtColorDlg();

// Dialog Data
    enum { IDD = IDD_FMT_COLORDLG };
    COLORREF m_colorDef ;

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //UINT_PTR CALLBACK CCHookProc(HWND hdlg, UINT uiMsg,WPARAM wParam,LPARAM lParam);
    DECLARE_MESSAGE_MAP()
public:
    BOOL m_bUseDef;
    BOOL m_bShowUseDefault;
    virtual BOOL OnInitDialog();
    afx_msg void OnBnClickedDefClr();

    virtual INT_PTR DoModal();
protected:
    virtual void OnOK();
};
