#pragma once
// ExportSplitterFrame.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CExportSplitterFrame frame
#include "FlatSplitterWnd.h"

class CDictTreeView;
class CExportOptionsView;
class CExportDoc;

class CExportSplitterFrame : public CFrameWnd
{
    DECLARE_DYNCREATE(CExportSplitterFrame)
public:
    CExportSplitterFrame();
    virtual ~CExportSplitterFrame();

    CDictTreeView*      GetLeftView();
    CExportOptionsView* GetRightView();

    CExportDoc* GetDoc();
    void        SetDoc( CExportDoc* pDoct );

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CExportSplitterFrame)
    protected:
    virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
    //}}AFX_VIRTUAL

// Implementation
protected:


    // Generated message map functions
    //{{AFX_MSG(CExportSplitterFrame)
    afx_msg void OnSize(UINT nType, int cx, int cy);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    CFlatSplitterWnd    m_cFlatSplitter;

    CDictTreeView*      m_pLeftView;
    CExportOptionsView* m_pRightView;

    CExportDoc*         m_pDoc;

public:
    bool                m_bSingleFile_Default;
    bool                m_bAllInOneRecord_Default;
    bool                m_bJoinSingleWithMultipleRecords_Default;
    int                 m_iExportRecordType_Default;
    int                 m_iExportItems_Default;
    int                 m_iExportFormat_Default;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
