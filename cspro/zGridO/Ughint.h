#pragma once

// UGHint.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CUGHint window


class CLASS_DECL_ZGRIDO CUGHint : public CWnd
{
// Construction
public:
    CUGHint();

// Attributes
public:

// Operations
public:

    BOOL Create(CWnd* pParentWnd, HBRUSH hbrBackground=NULL);
    // --- In  :    pParentWnd      -   pointer to parent window
    //              hbrBackground   -   brush to be used to fill background
    // --- Out :
    // --- Returns: TRUE if item tip window was successfully created, or FALSE
    //              otherwise
    // --- Effect : creates item tip window

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CUGHint)
    protected:
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CUGHint();

    // Generated message map functions
protected:
    //{{AFX_MSG(CUGHint)
    afx_msg void OnPaint();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()


public:

    CUGCtrl *       m_ctrl;     //pointer to the main class
    CString         m_text;
    COLORREF        m_textColor;
    COLORREF        m_backColor;
    int             m_windowAlign;
    int             m_textAlign;
    CFont *         m_hFont;
    int             m_fontHeight;

    int SetWindowAlign(int align);
    int SetTextAlign(int align);
    int SetTextColor(COLORREF color);
    int SetBackColor(COLORREF color);
    int SetFont(CFont *font);

    int SetText(LPCTSTR string,int update);
    int MoveHintWindow(int x,int y,int width);

    int Hide();
    int Show();

};

/////////////////////////////////////////////////////////////////////////////
