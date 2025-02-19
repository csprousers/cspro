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
    static std::string CreateWindowTitle(const std::string& spec_file_path, const std::string& dictionary_file_path);

    void OnInitialUpdate() override;

    void UpdateListViews();
    void UpdateForm();

    CSortDoc* GetDocument() { return assert_cast<CSortDoc*>(m_pDocument); }

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;

    void OnInsert();
    void OnDelete();
    void OnAscending();
    void OnDescending();
    void OnDeleteAll();
    void OnInsertAll();
    void OnMoveUp();
    void OnMoveDown();
    void OnClickItemsToSort(NMHDR* pNMHDR, LRESULT* pResult);
    void OnClickSortKeys(NMHDR* pNMHDR, LRESULT* pResult);
    void OnUpdateFileRun(CCmdUI* pCmdUI);
    void OnDblclkItemsToSort(NMHDR* pNMHDR, LRESULT* pResult);
    void OnDblclkSortKeys(NMHDR* pNMHDR, LRESULT* pResult);

private:
    CListCtrl m_SortKeys;
    CListCtrl m_SortItems;
    bool m_bFirst;
};
