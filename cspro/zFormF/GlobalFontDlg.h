#pragma once

class CFormScrollView;


class CGlobalFDlg : public CDialog
{
// Construction
public:
    CGlobalFDlg(CWnd* pParent = nullptr);   // standard constructor

    int m_iFont;
    std::string m_sCurFontDesc;

    LOGFONT m_lfDefault; //system default
    LOGFONT m_lfCurrentFont;//current font
    LOGFONT m_lfSelectedFont;//Selected font

    CFormScrollView* m_pFormView;

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
    BOOL OnInitDialog() override;

    afx_msg void OnRadio1();
    afx_msg void OnRadio2();
    afx_msg void OnFont();
    afx_msg void OnApply();
};
