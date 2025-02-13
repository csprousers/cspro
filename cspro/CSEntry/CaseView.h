#pragma once
/////////////////////////////////////////////////////////////////////////////
// CCaseView view

#include <afxcview.h>


struct NODEINFO
{
    const CaseSummary& case_summary;
    int iNode;
    int case_number;
};

class CCaseView : public CTreeView
{
protected:
    CCaseView();           // protected constructor used by dynamic creation
    DECLARE_DYNCREATE(CCaseView)

// Attributes
public:
    CImageList      m_cImageList;   // where my icons will be stored during registration
private:
    int             m_iCaseNumber;
// Operations
public:
    void BuildTree();
    void UpdateInsertText();

    int PartialSaveStatusModeToImageIndex(PartialSaveMode eMode) const;

    bool m_bSelChangeUpdate; // while selection changes during build tree
                             // Switch it off
    bool m_bAllowSel; //Allow tree selection during verify mode

    void SetCaseNumber(int iNumber) { m_iCaseNumber = iNumber; }
    int  GetSelectedCaseNumber();
    void RestoreSelectPos(const TCHAR* restore_using_key = nullptr);

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CCaseView)
    public:
    virtual void OnInitialUpdate();
    protected:
    virtual void OnDraw(CDC* pDC);      // overridden to draw this view
    //}}AFX_VIRTUAL

public:
    void BuildNodeTree(HTREEITEM hNodeItem);
    void InsertNode();
    void AddNode();
    void DeleteNode();
private:
    bool GetSelectedNode(HTREEITEM* phItem,NODEINFO** ppNodeInfo);
    void AddCaseToTree(const CaseSummary& case_summary, bool bShowLabels, bool bMultiLevel, int case_number);

public:
    void SetAllowSel(bool bSel) { m_bAllowSel = bSel; }
    bool GetAllowSel() { return m_bAllowSel; }
    bool IsLastCase();

    bool AllowSelChange(HTREEITEM hSelItem);

// Implementation
protected:
    virtual ~CCaseView();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

public:
    void OnDeleteCase();
private:
    void MainFrameCallerHelper(WPARAM wParam);

    // Generated message map functions
protected:
    //{{AFX_MSG(CCaseView)
    afx_msg void OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnSelchanging(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnKeydown(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDeleteitem(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnItemexpanding(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
    afx_msg UINT OnGetDlgCode();
    afx_msg void OnAdd();
    afx_msg void OnInsertCase();
    afx_msg void OnModifyCase();
    afx_msg void OnViewQuestionnaire();
    afx_msg void OnDblclk(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnFindCase();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
