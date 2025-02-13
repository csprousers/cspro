#pragma once
// CSButton.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCSButton window

class CCSButton : public CButton
{
// Construction
public:


    //Constructor
    CCSButton( );

    //Copy constructor
    CCSButton( CCSButton & x );

    //Operators
    void operator=   ( CCSButton & x );



// Attributes
public:



// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CCSButton)
    //}}AFX_VIRTUAL

// Implementation
public:

    //
    void  SetParent( CWnd* pParent);
    CWnd* GetParent();

    //
    void    SetLabel( CString CSLabel );
    CString GetLabel();

    //
    void SetIndex(int iIndex);
    int  GetIndex();

    //
    virtual ~CCSButton();



    // Generated message map functions
protected:
    //{{AFX_MSG(CCSButton)
    afx_msg void OnClicked();
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()
private:

    void Copy( CCSButton & x );

    CString m_CSLabel; //Texto del bto�n.
    int     m_iIndex ; //�ndice num�rico asociado.
    CWnd*   m_pParent; //Contenedor.
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
