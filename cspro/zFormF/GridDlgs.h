#pragma once


/////////////////////////////////////////////////////////////////////////////
// CGridBoxDlg dialog

class CGridBoxDlg : public CDialog
{
public:
    CGridBoxDlg(RosterOrientation roster_orientation, bool box_in_every_cell, CWnd* pParent = nullptr);

    enum { IDD = IDD_GRIDBOXPROP };

    bool GetBoxInEveryCell() const { return ( m_boxInEveryCell == 0 ); }

protected:
    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

private:
    const TCHAR* m_orientationText;
    int m_boxInEveryCell;
    CString m_dlgMessage;
};


/////////////////////////////////////////////////////////////////////////////
// CGridTextDlg dialog

class CGridTextDlg : public CDialog
{
// Construction
public:
    CGridTextDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
    //{{AFX_DATA(CGridTextDlg)
    enum { IDD = IDD_GRIDTEXTPROP };
    CString m_csMsg1;
    CString m_csTxt;
    int     m_iFont;
    int     m_iMulti;
    //}}AFX_DATA
    LOGFONT m_lfDefault;
    LOGFONT m_lfCustom;
    bool    m_bDisableTxt;
    CString m_csRadio1;
    CString m_csRadio2;
    COLORREF    m_color;
    bool                m_applytoall;

//    bool m_bHideRadioButtons;

private:
    bool m_bFontChanged;

// Operations
public:
    bool FontChanged(void) const { return m_bFontChanged; }

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CGridTextDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CGridTextDlg)
    virtual BOOL OnInitDialog();
    afx_msg void OnFont();
    afx_msg void OnRadiofont();
    afx_msg void OnRadiofont2();
    afx_msg void OnColor();
    afx_msg void OnApply();
    afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
    afx_msg void OnReset();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CJoinDlg dialog

class CJoinDlg : public CDialog
{
public:
    CJoinDlg(RosterOrientation roster_orientation, const CString& suggested_label, CWnd* pParent = nullptr);

    enum { IDD = IDD_JOIN };

    const CString& GetLabel() const { return m_label; }

protected:
    void DoDataExchange(CDataExchange* pDX) override;

private:
    CString m_label;
    CString m_dlgMessage;
};
