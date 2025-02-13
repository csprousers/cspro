#pragma once

// CPrtViewGotoPageDlg dialog

class CPrtViewGotoPageDlg : public CDialog
{
    DECLARE_DYNAMIC(CPrtViewGotoPageDlg)

public:
    CPrtViewGotoPageDlg(CWnd* pParent = NULL);   // standard constructor
    virtual ~CPrtViewGotoPageDlg();

// Dialog Data
    enum { IDD = IDD_PRTVIEW_GOTO_PAGE };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

    DECLARE_MESSAGE_MAP()
public:
    int m_iPage;
};


// CPrtViewGotoTblDlg dialog

class CPrtViewGotoTblDlg : public CDialog
{
    DECLARE_DYNAMIC(CPrtViewGotoTblDlg)

public:
    CPrtViewGotoTblDlg(CTabSet* pTabSet, CWnd* pParent = NULL);   // standard constructor
    virtual ~CPrtViewGotoTblDlg();

// Dialog Data
    enum { IDD = IDD_PRTVIEW_GOTO_TBL };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnInitDialog();

    DECLARE_MESSAGE_MAP()
public:
    int m_iTbl;
    CTabSet* m_pTabSet;
};

// CPrtViewGotoAreaDlg dialog

class CPrtViewGotoAreaDlg : public CDialog
{
    DECLARE_DYNAMIC(CPrtViewGotoAreaDlg)

public:
    CPrtViewGotoAreaDlg(CTabSet* pTabSet,
        CMapStringToString& areaLookup,
        CWnd* pParent = NULL);   // standard constructor
    virtual ~CPrtViewGotoAreaDlg();

// Dialog Data
    enum { IDD = IDD_PRTVIEW_GOTO_AREA };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnInitDialog();
    void AddAreaNamesToCombo(int iTbl);
    int m_iArea;
    CMapStringToString& m_areaLookup;
    CString GetAreaLabel(int iTbl, int iArea, bool bIndent) const;

    DECLARE_MESSAGE_MAP()
public:
    int m_iTbl;
    CTabSet* m_pTabSet;
    afx_msg void OnCbnSelchangeTbl();
    CString GetArea() const;
    void SetArea(LPCTSTR sArea);
};
