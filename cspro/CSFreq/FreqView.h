#pragma once

// FreqView.h : interface of the CSFreqView class
//
/////////////////////////////////////////////////////////////////////////////

#include <CSFreq/FreqDoc.h>
#include <CSFreq/Ddtrctl.h>


class CSFreqView : public CFormView
{
    DECLARE_DYNCREATE(CSFreqView)

protected:
    CSFreqView(); // create from serialization only

public:
    enum { IDD = IDD_FREQ_FORM };

    CSFreqDoc* GetDocument() { return (CSFreqDoc*)m_pDocument; }

public:
    void RefreshTree();

protected:
    DECLARE_MESSAGE_MAP()

    BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL) override;
    void OnInitialUpdate() override;
    void DoDataExchange(CDataExchange* pDX) override;
    void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView) override;

    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnClickDatadictTree(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnKeydownDatadictTree(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg LRESULT OnTvCheckbox(WPARAM wp, LPARAM lp);

public:
    void SetParentStates(HTREEITEM hItem);
    CString GetDetails(HTREEITEM hItem);
    bool AddRecordIntree(const CDictRecord* pRecord,HTREEITEM htreeLabel);
    void TreeItemClicked(HTREEITEM hItem, int recursive = 0);
    CFreqDDTreeCtrl* GetDictTree() { return &m_dicttree;}

private:
    void InitializeView();

private:
    CFreqDDTreeCtrl m_dicttree;
    CImageList      m_ilState;
    CImageList      m_ilNormal;
};
