//***************************************************************************
//  File name: SortView.cpp
//
//  Description:
//       CSSort view implementation
//
//  History:    Date       Author   Comment
//              ---------------------------
//              21 Nov 00   bmd     Created for CSPro 2.1
//
//***************************************************************************

#include "StdAfx.h"
#include "SortView.h"
#include "SortDoc.h"


IMPLEMENT_DYNCREATE(CSortView, CFormView)


BEGIN_MESSAGE_MAP(CSortView, CFormView)
    ON_BN_CLICKED(IDC_INSERT, OnInsert)
    ON_BN_CLICKED(IDC_DELETE, OnDelete)
    ON_BN_CLICKED(IDC_ASCENDING, OnAscending)
    ON_BN_CLICKED(IDC_DESCENDING, OnDescending)
    ON_BN_CLICKED(IDC_DELETE_ALL, OnDeleteAll)
    ON_BN_CLICKED(IDC_INSERT_ALL, OnInsertAll)
    ON_BN_CLICKED(IDC_MOVE_UP, OnMoveUp)
    ON_BN_CLICKED(IDC_MOVE_DOWN, OnMoveDown)
    ON_NOTIFY(NM_CLICK, IDC_ITEMS_TO_SORT, OnClickItemsToSort)
    ON_NOTIFY(NM_CLICK, IDC_SORT_KEYS, OnClickSortKeys)
    ON_UPDATE_COMMAND_UI(ID_FILE_RUN, OnUpdateFileRun)
    ON_NOTIFY(NM_DBLCLK, IDC_ITEMS_TO_SORT, OnDblclkItemsToSort)
    ON_NOTIFY(NM_DBLCLK, IDC_SORT_KEYS, OnDblclkSortKeys)
END_MESSAGE_MAP()


CSortView::CSortView()
    :   CFormView(IDD_DATASORT_FORM),
        m_bFirst(true)
{
}


std::string CSortView::CreateWindowTitle(const std::string& spec_file_path, const std::string& dictionary_file_path)
{
    return FormatText("CSPro Sort Data - [Spec File = %s / Dictionary = %s]",
                      !spec_file_path.empty() ? PortableFunctions::PathGetFilename(spec_file_path).c_str() : "Untitled",
                      PortableFunctions::PathGetFilename(dictionary_file_path).c_str());
}


void CSortView::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_SORT_KEYS, m_SortKeys);
    DDX_Control(pDX, IDC_ITEMS_TO_SORT, m_SortItems);
}


void CSortView::OnInsert()
{
    CSortDoc* pDoc = GetDocument();
    pDoc->SetModifiedFlag();
    ASSERT (pDoc != NULL);
    int i;
    int iItem = -1;
    for (i = 0 ; i < (int) m_SortItems.GetSelectedCount() ; i++) {
        iItem = m_SortItems.GetNextItem(iItem, LVNI_SELECTED);
        int iAvail = pDoc->m_aAvail.ElementAt(iItem - i);       // BMD 19 Dec 2002
        pDoc->m_aItem[iAvail].order = SortSpec::SortOrder::Ascending;
        pDoc->m_aKey.Add(pDoc->m_aAvail.ElementAt(iItem - i));
        pDoc->m_aAvail.RemoveAt(iItem - i);
    }
    UpdateListViews();
    if (m_SortItems.GetItemCount() > 0) {
        m_SortItems.SetItemState(0, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
        UpdateForm();
        GetDlgItem(IDC_ITEMS_TO_SORT)->SetFocus();
    }
    else {
        m_SortKeys.SetItemState(0, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
        UpdateForm();
        GetDlgItem(IDC_SORT_KEYS)->SetFocus();
    }
}


void CSortView::OnDelete()
{
    CSortDoc* pDoc = GetDocument();
    pDoc->SetModifiedFlag();
    ASSERT (pDoc != NULL);
    int i, j;
    int iItem = -1;
    int iKey;
    for (i = 0 ; i < (int) m_SortKeys.GetSelectedCount() ; i++) {
        iItem = m_SortKeys.GetNextItem(iItem, LVNI_SELECTED);
        iKey = pDoc->m_aKey.ElementAt(iItem - i);
        for (j = 0 ; j < (int) pDoc->m_aAvail.GetSize() ; j++) {
            if (iKey < pDoc->m_aAvail.ElementAt(j)) {
                break;
            }
        }
        pDoc->m_aAvail.InsertAt(j, iKey);
        pDoc->m_aKey.RemoveAt(iItem - i);
    }
    UpdateListViews();
    if (m_SortKeys.GetItemCount() > 0) {
        m_SortKeys.SetItemState(0, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
        UpdateForm();
        GetDlgItem(IDC_SORT_KEYS)->SetFocus();
    }
    else {
        m_SortItems.SetItemState(0, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
        UpdateForm();
        GetDlgItem(IDC_ITEMS_TO_SORT)->SetFocus();
    }
}


void CSortView::OnInsertAll()
{
    CSortDoc* pDoc = GetDocument();
    pDoc->SetModifiedFlag();
    ASSERT (pDoc != NULL);
    while (pDoc->m_aAvail.GetSize() > 0) {
        int iAvail = pDoc->m_aAvail.ElementAt(0);       // BMD 19 Dec 2002
        pDoc->m_aItem[iAvail].order = SortSpec::SortOrder::Ascending;
        pDoc->m_aKey.Add(pDoc->m_aAvail.ElementAt(0));
        pDoc->m_aAvail.RemoveAt(0);
    }
    UpdateListViews();
    GetDlgItem(IDC_SORT_KEYS)->SetFocus();
    m_SortKeys.SetItemState(0, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
    UpdateForm();
    GetDlgItem(IDC_SORT_KEYS)->SetFocus();
}


void CSortView::OnDeleteAll()
{
    CSortDoc* pDoc = GetDocument();
    pDoc->SetModifiedFlag();
    ASSERT (pDoc != NULL);
    int i, iKey;
    while (pDoc->m_aKey.GetSize() > 0) {
        iKey = pDoc->m_aKey.ElementAt(0);
        for (i = 0 ; i < (int) pDoc->m_aAvail.GetSize() ; i++) {
            if (iKey < pDoc->m_aAvail.ElementAt(i)) {
                break;
            }
        }
        pDoc->m_aAvail.InsertAt(i, iKey);
        pDoc->m_aKey.RemoveAt(0);
    }
    UpdateListViews();
    m_SortItems.SetItemState(0, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
    UpdateForm();
    GetDlgItem(IDC_ITEMS_TO_SORT)->SetFocus();
}


void CSortView::OnMoveUp()
{
    CSortDoc* pDoc = GetDocument();
    pDoc->SetModifiedFlag();
    ASSERT (pDoc != NULL);
    int i, iKey;
    i = m_SortKeys.GetNextItem(-1, LVNI_SELECTED);
    iKey = pDoc->m_aKey.ElementAt(i);
    pDoc->m_aKey.RemoveAt(i);
    i--;
    pDoc->m_aKey.InsertAt(i, iKey);
    UpdateListViews();
    m_SortKeys.SetItemState(i, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
    UpdateForm();
    GetDlgItem(IDC_SORT_KEYS)->SetFocus();
}


void CSortView::OnMoveDown()
{
    CSortDoc* pDoc = GetDocument();
    pDoc->SetModifiedFlag();
    ASSERT (pDoc != NULL);
    int i = -1;
    int j, iKey;
    for (j = 0 ; j < (int) m_SortKeys.GetSelectedCount() ; j++) {
        i = m_SortKeys.GetNextItem(i, LVNI_SELECTED);
    }
    iKey = pDoc->m_aKey.ElementAt(i);
    pDoc->m_aKey.RemoveAt(i);
    i++;
    pDoc->m_aKey.InsertAt(i, iKey);
    UpdateListViews();
    m_SortKeys.SetItemState(i, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
    UpdateForm();
    GetDlgItem(IDC_SORT_KEYS)->SetFocus();
}


void CSortView::OnAscending()
{
    CSortDoc* pDoc = GetDocument();
    pDoc->SetModifiedFlag();
    ASSERT (pDoc != NULL);
    int i, iKey;
    int iItem = -1;
    for (i = 0 ; i < (int) m_SortKeys.GetSelectedCount() ; i++) {
        iItem = m_SortKeys.GetNextItem(iItem, LVNI_SELECTED);
        iKey = pDoc->m_aKey.ElementAt(iItem);
        pDoc->m_aItem[iKey].order = SortSpec::SortOrder::Ascending;
    }
    UpdateListViews();
    m_SortKeys.SetItemState(iItem, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
    UpdateForm();
    GetDlgItem(IDC_SORT_KEYS)->SetFocus();
}


void CSortView::OnDescending()
{
    CSortDoc* pDoc = GetDocument();
    pDoc->SetModifiedFlag();
    ASSERT (pDoc != NULL);
    int i;
    int iItem = -1;
    int iKey;
    for (i = 0 ; i < (int) m_SortKeys.GetSelectedCount() ; i++) {
        iItem = m_SortKeys.GetNextItem(iItem, LVNI_SELECTED);
        iKey = pDoc->m_aKey.ElementAt(iItem);
        pDoc->m_aItem[iKey].order = SortSpec::SortOrder::Descending;
    }
    UpdateListViews();
    m_SortKeys.SetItemState(iItem, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
    UpdateForm();
    GetDlgItem(IDC_SORT_KEYS)->SetFocus();
}


void CSortView::OnInitialUpdate()
{
    __super::OnInitialUpdate();

    ASSERT (GetDocument() != NULL);

    if (m_bFirst) {
        CRect clientRect, viewRect, questRect, sortRect;
        GetClientRect(&clientRect);
        GetParentFrame()->GetWindowRect(&viewRect);
        GetDlgItem(IDC_ITEMS_TO_SORT)->GetWindowRect(&questRect);
        GetDlgItem(IDC_SORT_KEYS)->GetWindowRect(&sortRect);
        viewRect.bottom = sortRect.bottom + 68;
        viewRect.right  = sortRect.right + 26;
        GetParentFrame()->MoveWindow(&viewRect);
        GetParentFrame()->CenterWindow();
    }

    m_SortItems.DeleteAllItems();
    m_SortKeys.DeleteAllItems();
    CSortDoc* pDoc = GetDocument();

    for (int i = 0 ; i < pDoc->m_aAvail.GetSize() ; i++)  {
        m_SortItems.InsertItem(i, pDoc->m_aItem[pDoc->m_aAvail[i]].dict_item->GetLabel());
    }

    GetParent()->SetWindowText(TC::ToWide(CreateWindowTitle(pDoc->GetSpecFilePath(), pDoc->GetDictionaryFilePath())).c_str());

    if (m_bFirst) {
        m_bFirst = FALSE;
        CRect rect;
        GetDlgItem(IDC_ITEMS_TO_SORT)->GetWindowRect(&rect);
        m_SortItems.InsertColumn(0, _T("Items"), LVCFMT_LEFT, rect.Width() - 4);
        GetDlgItem(IDC_SORT_KEYS)->GetWindowRect(&rect);
        int iWidth = rect.Width() - 4;
        m_SortKeys.InsertColumn(0, _T("Keys"), LVCFMT_LEFT, (3 * iWidth) / 5);
        m_SortKeys.InsertColumn(1, _T("Order"), LVCFMT_LEFT, (2 * iWidth) / 5);
    }
    UpdateListViews();
    m_SortItems.SetItemState(0, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
    UpdateForm();
    GetDlgItem(IDC_ITEMS_TO_SORT)->SetFocus();
}


void CSortView::OnClickItemsToSort(NMHDR* /*pNMHDR*/, LRESULT* const pResult)
{
    m_SortKeys.DeleteAllItems();
    CSortDoc* pDoc = GetDocument();
    for (int i = 0 ; i < pDoc->m_aKey.GetSize() ; i++)  {
        m_SortKeys.InsertItem(i, pDoc->m_aItem[pDoc->m_aKey[i]].dict_item->GetLabel());
        if (pDoc->m_aItem[pDoc->m_aKey[i]].order == SortSpec::SortOrder::Ascending) {
            m_SortKeys.SetItemText(i, 1, _T("Ascending"));
        }
        else {
            m_SortKeys.SetItemText(i, 1, _T("Descending"));
        }
    }
    UpdateData(FALSE);
    UpdateForm();
    *pResult = 0;
}


void CSortView::OnClickSortKeys(NMHDR* /*pNMHDR*/, LRESULT* const pResult)
{
    m_SortItems.DeleteAllItems();
    CSortDoc* pDoc = GetDocument();
    for (int i = 0 ; i < pDoc->m_aAvail.GetSize() ; i++)  {
        m_SortItems.InsertItem(i, pDoc->m_aItem[pDoc->m_aAvail[i]].dict_item->GetLabel());
    }
    UpdateData(FALSE);
    UpdateForm();
    *pResult = 0;
}


void CSortView::OnDblclkItemsToSort(NMHDR* /*pNMHDR*/, LRESULT* const pResult)
{
    OnInsert();
    *pResult = 0;
}


void CSortView::OnDblclkSortKeys(NMHDR* /*pNMHDR*/, LRESULT* const pResult)
{
    OnDelete();
    *pResult = 0;
}


void CSortView::OnUpdateFileRun(CCmdUI* const pCmdUI)
{
    pCmdUI->Enable(( m_SortKeys.GetItemCount() > 0 ));
}


void CSortView::UpdateListViews()
{
    m_SortItems.DeleteAllItems();
    m_SortKeys.DeleteAllItems();
    CSortDoc* pDoc = GetDocument();
    int i;
    for (i = 0 ; i < pDoc->m_aAvail.GetSize() ; i++)  {
        m_SortItems.InsertItem(i, pDoc->m_aItem[pDoc->m_aAvail[i]].dict_item->GetLabel());
    }
    for (i = 0 ; i < pDoc->m_aKey.GetSize() ; i++)  {
        m_SortKeys.InsertItem(i, pDoc->m_aItem[pDoc->m_aKey[i]].dict_item->GetLabel());
        if (pDoc->m_aItem[pDoc->m_aKey[i]].order == SortSpec::SortOrder::Ascending) {
            m_SortKeys.SetItemText(i, 1, _T("Ascending"));
        }
        else {
            m_SortKeys.SetItemText(i, 1, _T("Descending"));
        }
    }
}


void CSortView::UpdateForm()
{
    if (m_SortItems.GetItemCount() > 0) {
        GetDlgItem(IDC_ITEMS_TO_SORT)->EnableWindow(TRUE);
        GetDlgItem(IDC_INSERT_ALL)->EnableWindow(TRUE);
        if (m_SortItems.GetSelectedCount() > 0) {
            GetDlgItem(IDC_INSERT)->EnableWindow(TRUE);
        }
        else {
            GetDlgItem(IDC_INSERT)->EnableWindow(FALSE);
        }
    }
    else {
        GetDlgItem(IDC_ITEMS_TO_SORT)->EnableWindow(TRUE);
        GetDlgItem(IDC_INSERT)->EnableWindow(FALSE);
        GetDlgItem(IDC_INSERT_ALL)->EnableWindow(FALSE);
    }
    if (m_SortKeys.GetItemCount() > 0) {
        GetDlgItem(IDC_SORT_KEYS)->EnableWindow(TRUE);
        GetDlgItem(IDC_DELETE_ALL)->EnableWindow(TRUE);
        if (m_SortKeys.GetSelectedCount() > 0) {
            GetDlgItem(IDC_DELETE)->EnableWindow(TRUE);
            int i = m_SortKeys.GetNextItem(-1, LVNI_SELECTED);
            if (i > 0) {
                GetDlgItem(IDC_MOVE_UP)->EnableWindow(TRUE);
            }
            else {
                GetDlgItem(IDC_MOVE_UP)->EnableWindow(FALSE);
            }
            i = -1;
            for (int j = 0 ; j < (int) m_SortKeys.GetSelectedCount() ; j++) {
                i = m_SortKeys.GetNextItem(i, LVNI_SELECTED);
            }
            if (i < m_SortKeys.GetItemCount() - 1) {
                GetDlgItem(IDC_MOVE_DOWN)->EnableWindow(TRUE);
            }
            else {
                GetDlgItem(IDC_MOVE_DOWN)->EnableWindow(FALSE);
            }
            GetDlgItem(IDC_ASCENDING)->EnableWindow(TRUE);
            GetDlgItem(IDC_DESCENDING)->EnableWindow(TRUE);
        }
        else {
            GetDlgItem(IDC_DELETE)->EnableWindow(FALSE);
            GetDlgItem(IDC_MOVE_UP)->EnableWindow(FALSE);
            GetDlgItem(IDC_MOVE_DOWN)->EnableWindow(FALSE);
            GetDlgItem(IDC_ASCENDING)->EnableWindow(FALSE);
            GetDlgItem(IDC_DESCENDING)->EnableWindow(FALSE);
        }
    }
    else {
        GetDlgItem(IDC_SORT_KEYS)->EnableWindow(FALSE);
        GetDlgItem(IDC_DELETE)->EnableWindow(FALSE);
        GetDlgItem(IDC_DELETE_ALL)->EnableWindow(FALSE);
        GetDlgItem(IDC_MOVE_UP)->EnableWindow(FALSE);
        GetDlgItem(IDC_MOVE_DOWN)->EnableWindow(FALSE);
        GetDlgItem(IDC_ASCENDING)->EnableWindow(FALSE);
        GetDlgItem(IDC_DESCENDING)->EnableWindow(FALSE);
    }
}
