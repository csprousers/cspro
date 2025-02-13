#pragma once

//***************************************************************************
//  File name: SortView.h
//
//  Description:
//       Header for CSSort view
//
//  History:    Date       Author   Comment
//              ---------------------------
//              21 Nov 00   bmd     Created for CSPro 2.1
//
//***************************************************************************

#include <CSSort/SortDoc.h>


class CSortView : public CFormView
{
    DECLARE_DYNCREATE(CSortView)

protected:
    CSortView(); // create from serialization only

public:
    enum { IDD = IDD_DATASORT_FORM };

    virtual ~CSortView();

    static CString CreateWindowTitle(const TCHAR* spec_filename, const TCHAR* dictionary_filename);

    void OnInitialUpdate() override;

    void UpdateListViews();
    void UpdateForm();

    CSortDoc* GetDocument()
    {
        ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CSortDoc)));
        return static_cast<CSortDoc*>(m_pDocument);
    }

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX)  override;

    afx_msg void OnInsert();
    afx_msg void OnDelete();
    afx_msg void OnAscending();
    afx_msg void OnDescending();
    afx_msg void OnDeleteAll();
    afx_msg void OnInsertAll();
    afx_msg void OnMoveUp();
    afx_msg void OnMoveDown();
    afx_msg void OnClickItemsToSort(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnClickSortKeys(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnUpdateFileRun(CCmdUI* pCmdUI);
    afx_msg void OnDblclkItemsToSort(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDblclkSortKeys(NMHDR* pNMHDR, LRESULT* pResult);

private:
    CListCtrl m_SortKeys;
    CListCtrl m_SortItems;
    bool m_bFirst;
};
