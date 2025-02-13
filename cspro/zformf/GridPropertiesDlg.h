#pragma once


/////////////////////////////////////////////////////////////////////////////
// CGridProp dialog

class CGridProp : public CDialog
{
// Construction
public:
    CGridProp(CWnd* pParent = NULL);   // standard constructor
    CGridProp(CDERoster* pRoster, CFormScrollView* pParent);

    RosterOrientation GetRosterOrientation() const;
    FreeMovement GetFreeMovement() const;

// Dialog Data
    //{{AFX_DATA(CGridProp)
    enum { IDD = IDD_GRIDPROP };

    CFormScrollView* m_pMyParent;
    CDERoster*       m_pRoster;

    CIMSAString m_sGridName;
    CIMSAString m_sGridLabel;
    CWnd    m_HorzWnd;
    CWnd    m_VertWnd;
    CIMSAString m_sMaxOccField;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CGridProp)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    DECLARE_MESSAGE_MAP()

    // Generated message map functions
    //{{AFX_MSG(CGridProp)
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    afx_msg void OnHorz();
    afx_msg void OnVert();
    afx_msg void OnFreemove();
    //}}AFX_MSG

private:
    int m_iHorz;
    BOOL m_bFreemovement;
    int m_iFreemovement;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
