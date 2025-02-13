#pragma once

// ExportOptionsView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CExportOptionsView form view

#include <zEdit2O/LogicCtrl.h>

class CExportSplitterFrame;
class CExportDoc;


#define ExportOptionsView_ExportFormat_TabDelimited     0
#define ExportOptionsView_ExportFormat_CommaDelimited   1
#define ExportOptionsView_ExportFormat_SemiColon        2
#define ExportOptionsView_ExportFormat_CsPro            3
#define ExportOptionsView_ExportFormat_SPSS             4
#define ExportOptionsView_ExportFormat_SAS              5
#define ExportOptionsView_ExportFormat_STATA            6
#define ExportOptionsView_ExportFormat_R                7
#define ExportOptionsView_ExportFormat_SPSS_SAS_STATA_R 8


class CExportOptionsView : public CFormView
{
protected:
    CExportOptionsView();           // protected constructor used by dynamic creation
    DECLARE_DYNCREATE(CExportOptionsView)

    void        SetDoc( CExportDoc* pDoc );
    CExportDoc* GetDoc();

    void    FromDoc ( CExportDoc* pDoc );
    void    ToDoc   ( CExportDoc* pDoc );

    void    EnableJoinSingleMultiple(bool bEnable);


// Form Data
public:
    enum { IDD = IDD_EXPORT_OPTIONS };

// Attributes
public:
    bool IsJoinEnabled();
    void UpdateEnabledDisabledCtrlsStatus(bool* pbJoinValue = NULL );
    bool CheckUniverseSyntax(const CString& sUniverseStatement);

    void RefreshLexer();

// Overrides
protected:
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CExportOptionsView)
    void OnInitialUpdate() override;
    void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    virtual ~CExportOptionsView();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

protected:
    DECLARE_MESSAGE_MAP()

    // Generated message map functions
    afx_msg void OnChangeEditUniverse();
    afx_msg void OnEditUniverseButton();
    afx_msg void OnSelectionBasic();
    afx_msg void OnSelectionCheckFileExtension();

    void CheckFileExtension();
    void Gui2Data();
    void Data2Gui();

    bool                m_bSingleFile;
    bool                m_bAllInOneRecord;
    bool                m_bJoinSingleWithMultipleRecords;
    int                 m_iExportRecordType;
    int                 m_iExportItems;
    int                 m_iExportFormat;
    CString             m_csUniverse;

    bool                m_bForceANSI; // GHM 20120416
    bool                m_bCommaDecimal;

    CExportDoc*         m_pDoc;

private:
    CLogicCtrl m_cEditUniverse;
};
