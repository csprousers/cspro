#pragma once
// Page2.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPage2 dialog
class CCapiRunAplEntry;
class CCaseTree;
class CMainFrame;
class CPage2 : public CPropertyPage
{
        DECLARE_DYNCREATE(CPage2)

// Construction
public:

        void            SetMainFrame( CMainFrame* pMainFrame );

        CCaseTree*      GetCaseTree();
        bool            CreateCaseTree();
        bool            DeleteCaseTree();

        void            DummyMove();

        CPage2();
        ~CPage2();

// Dialog Data
        //{{AFX_DATA(CPage2)
        enum { IDD = IDD_PROPPAGE2 };
                // NOTE - ClassWizard will add data members here.
                //    DO NOT EDIT what you see in these blocks of generated code !
        //}}AFX_DATA


// Overrides
        // ClassWizard generate virtual function overrides
        //{{AFX_VIRTUAL(CPage2)
        protected:
        virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
        //}}AFX_VIRTUAL

// Implementation
protected:
        // Generated message map functions
        //{{AFX_MSG(CPage2)
        afx_msg void OnSize(UINT nType, int cx, int cy);
        afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
        //}}AFX_MSG
        DECLARE_MESSAGE_MAP()

        LRESULT OnGivenGoTo                                     (WPARAM wParam, LPARAM lParam);
        LRESULT OnGoToNode                                      (WPARAM wParam, LPARAM lParam);
        LRESULT OnTreeItemWithNothingToDo       (WPARAM wParam, LPARAM lParam);
    LRESULT OnRestoreEntryRunViewFocus  (WPARAM wParam, LPARAM lParam);

    LRESULT OnDeleteCaseTree            (WPARAM wParam, LPARAM lParam);

    LRESULT OnUnknownKey            (WPARAM wParam, LPARAM lParam);
    LRESULT OnRecalcLayout          (WPARAM wParam, LPARAM lParam);


private:

        CCapiRunAplEntry*       m_pCapiRunAplEntry;
        CCaseTree*                      m_pCaseTree;
        CMainFrame*                     m_pMainFrame;
        int                                     m_iPageWidth;
        int                                     m_iPageHeight;
        int                     m_icx, m_icy;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
