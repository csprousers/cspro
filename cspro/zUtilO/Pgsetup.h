#pragma once
//***************************************************************************
//  File name: PGSETUP.H
//
//  Description:
//       Header for CPageMetrics
//                  CFolio
//                  CIMSAPageSetupDlg
//
//  History:    Date       Author   Comment
//              ---------------------------
//              21 Jan 98   bmd     Create from IMPS 4.1
//
//***************************************************************************
//
//  class CPageMetrics
//
//  Description
//      Stores margins and user area information
//
//  Implementation
//      GetUserMargins          Get user margins in twips.
//      GetUserArea             Get user print area in twips.
//      UpdateUserMargins       Updates the page metrics based on new user margins.
//
//***************************************************************************
//
//  CRect CPageMetrics::GetUserMargins();
//
//      Return value
//          User defined margins in twips from edge of page.
//
//---------------------------------------------------------------------------
//
//  CRect CPageMetrics::GetUserArea();
//
//      Return value
//          The print area rectangle in twips within the page hard margins.
//
//---------------------------------------------------------------------------
//
//  void CPageMetrics::UpdateUserMargins(CRect& rectUser);
//
//      Parameters
//          rectUser            The user margins in twips from the edge of the page.
//
//      Remarks
//          Will not allow the user margins to go outside the hard margins.
//
//***************************************************************************
//***************************************************************************
//***************************************************************************
//
//  class CFolio
//
//  Description:
//      Stores heading and footing (folio) information
//
//  Construction
//       CFolio                 Constructs folio object.
//
//  Attributes
//      HasHeader               Header is present.
//      GetHeaderLeft           Get left-hand header text.
//      GetHeaderCenter         Get center header text.
//      GetHeaderRight          Get right-hand header text.
//      HasFooter               Footer is present.
//      GetFooterLeft           Get left-hand footer text.
//      GetFooterCenter         Get center footer text.
//      GetFooterRight          Get right-hand footer text.
//      GetUserArea             Get user area for print or display.
//
//  Operations
//      Create                  Build contents from *.ini file.
//
//***************************************************************************
//
//  BOOL  CFolio::HasHeader(void);
//
//      Return value
//          TRUE if any headers are present, otherwise FALSE.
//
//---------------------------------------------------------------------------
//
//  CIMSAString CFolio::GetHeaderLeft(int iPage);
//
//      Parameters
//          iPage               The current page number.
//
//      Return value
//          The left-hand header text.
//
//---------------------------------------------------------------------------
//
//  CIMSAString CFolio::GetHeaderCenter(int iPage);
//
//      Parameters
//          iPage               The current page number.
//
//      Return value
//          The center header text.
//
//---------------------------------------------------------------------------
//
//  CIMSAString CFolio::GetHeaderRight(int iPage);
//
//      Parameters
//          iPage               The current page number.
//
//      Return value
//          The right-hand header text.
//
//---------------------------------------------------------------------------
//
//  BOOL  CFolio::HasFooter();
//
//      Return value
//          TRUE if any footers area present, otherwise FALSE.
//
//---------------------------------------------------------------------------
//
//  CIMSAString CFolio::GetFooterLeft(int iPage);
//
//      Parameters
//          iPage               The current page number.
//
//      Return value
//          The left-hand header text.
//
//---------------------------------------------------------------------------
//
//  CIMSAString CFolio::GetFooterCenter(int iPage);
//
//      Parameters
//          iPage               The current page number.
//
//      Return value
//          The left-hand header text.
//
//---------------------------------------------------------------------------
//
//  CIMSAString CFolio::GetFooterRight(int iPage);
//
//      Parameters
//          iPage               The current page number.
//
//      Return value
//          The left-hand header text.
//
//---------------------------------------------------------------------------
//
//  CRect CFolio::GetUserArea();
//
//---------------------------------------------------------------------------
//
//  void CFolio::Create(CIMSAString csFileName);
//
//***************************************************************************
//***************************************************************************
//***************************************************************************
//
//  class CIMSAPageSetupDlg : CDialog
//
//  Description:
//      Page setup dialog box.
//
//  Implementation
//      ComputeMetrics          Recomputes the contents of the CPageMetrics object.
//
//***************************************************************************
//
//***************************************************************************
//***************************************************************************
//***************************************************************************
#include <zUtilO/zUtilO.h>
#include <zUtilO/imsaStr.h>


/////////////////////////////////////////////////////////////////////////////
//
//                             CPageMetrics
//
/////////////////////////////////////////////////////////////////////////////

class CLASS_DECL_ZUTILO CPageMetrics {

// Data Members
private:
    CRect   m_rectUserMargins;          // User margins in twips from edge of page
    CRect   m_rectUserArea;             // User area coordinates in twips within hard margins

// Methods
public:
// Construction
    CPageMetrics() {}

// Implementation
    CRect GetUserMargins() {return m_rectUserMargins;}
    CRect GetUserArea() {return m_rectUserArea;}
    void UpdateUserMargins(CRect& rectUser);

private:
    CDC* CreatePrinterDC();
};


/////////////////////////////////////////////////////////////////////////////
//
//                                    CFolio
//
/////////////////////////////////////////////////////////////////////////////

class CLASS_DECL_ZUTILO CFolio {

// Data Members
private:
    BOOL        m_bHeader;                  // Has header
    CIMSAString m_csHeaderLeft;             // Header left text
    CIMSAString m_csHeaderCenter;           // Header center text
    CIMSAString m_csHeaderRight;            // Header right text

    BOOL        m_bFooter;                  // Has footer
    CIMSAString m_csFooterLeft;             // Footer left text
    CIMSAString m_csFooterCenter;           // Footer center text
    CIMSAString m_csFooterRight;            // Footer right text

    CIMSAString m_csFileName;               // File name

    CRect   m_rectMargins;                  // User margins in twips from edge of page
    CRect   m_rectUserArea;                 // User area coordinates in twips within hard margins

// Methods
public:
// Construction
    CFolio() {}

// Attributes
    BOOL  HasHeader(void) {return m_bHeader;}
    CIMSAString GetHeaderLeft(int iPage) {return SetPage(m_csHeaderLeft, iPage);}
    CIMSAString GetHeaderCenter(int iPage) {return SetPage(m_csHeaderCenter, iPage);}
    CIMSAString GetHeaderRight(int iPage) {return SetPage(m_csHeaderRight, iPage);}
    BOOL  HasFooter() {return m_bFooter;}
    CIMSAString GetFooterLeft(int iPage) {return SetPage(m_csFooterLeft, iPage);}
    CIMSAString GetFooterCenter(int iPage) {return SetPage(m_csFooterCenter, iPage);}
    CIMSAString GetFooterRight(int iPage) {return SetPage(m_csFooterRight, iPage);}
    CRect GetUserArea() {return m_rectUserArea;}

// Operations
   void Create(CIMSAString csFileName);

// Static
   static CIMSAString ExpandText(const CIMSAString& csText, const CIMSAString& csFileName, int iPage);

private:
    void SetDateTime(CIMSAString& cs);                  // Expand date, time, and file into string
    CIMSAString SetPage(CIMSAString cs, int iPage);     // Expand page number into string
};


/////////////////////////////////////////////////////////////////////////////
//
//                              CIMSAPageSetupDlg
//
/////////////////////////////////////////////////////////////////////////////

class CLASS_DECL_ZUTILO CIMSAPageSetupDlg : public CDialog {

// Data Members
private:
    CPageMetrics    m_pageMetrics;
    CRect           m_rectMargins;
    UINT            m_uLastControl;

// Dialog Data
    //{{AFX_DATA(CIMSAPageSetupDlg)
    CIMSAString m_csFooterCenter;
    CIMSAString m_csFooterLeft;
    CIMSAString m_csFooterRight;
    CIMSAString m_csHeaderCenter;
    CIMSAString m_csHeaderLeft;
    CIMSAString m_csHeaderRight;
    float   m_fMarginTop;
    float   m_fMarginBottom;
    float   m_fMarginLeft;
    float   m_fMarginRight;
    //}}AFX_DATA

// Methods
public:
// Construction
    CIMSAPageSetupDlg(CWnd* pParent = NULL);   // standard constructor

// Implemention
    void ComputeMetrics(void);

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CIMSAPageSetupDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

protected:
    void OnSetfocusHeadFoot(void);
    void OnKillfocusHeadFoot(void);

    // Generated message map functions
    //{{AFX_MSG(CIMSAPageSetupDlg)
    afx_msg void OnButtonDate();
    virtual BOOL OnInitDialog();
    afx_msg void OnButtonTime();
    afx_msg void OnButtonFile();
    afx_msg void OnButtonPage();
    afx_msg void OnSetfocusMarginLeft();
    afx_msg void OnChangeHeaderLeft();
    afx_msg void OnChangeHeaderCenter();
    afx_msg void OnChangeHeaderRight();
    afx_msg void OnChangeFooterLeft();
    afx_msg void OnChangeFooterCenter();
    afx_msg void OnChangeFooterRight();
    afx_msg void OnSetfocusMarginTop();
    afx_msg void OnSetfocusMarginBottom();
    afx_msg void OnSetfocusMarginRight();
    virtual void OnOK();
    afx_msg void OnKillfocusMarginLeft();
    afx_msg void OnKillfocusMarginRight();
    afx_msg void OnKillfocusMarginTop();
    afx_msg void OnKillfocusMarginBottom();
    afx_msg void OnSetfocusHeaderLeft();
    afx_msg void OnSetfocusHeaderCenter();
    afx_msg void OnSetfocusHeaderRight();
    afx_msg void OnSetfocusFooterLeft();
    afx_msg void OnSetfocusFooterCenter();
    afx_msg void OnSetfocusFooterRight();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};
