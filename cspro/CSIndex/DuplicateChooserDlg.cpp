#include "stdafx.h"
#include "DuplicateChooserDlg.h"
#include <afxpriv.h>


BEGIN_MESSAGE_MAP(DuplicateChooserDlg, CDialog)
    ON_COMMAND(ID_HELP, OnHelp)
    ON_BN_CLICKED(IDC_DLG_HELP, OnHelp)
    ON_NOTIFY(TVN_SELCHANGED, IDC_TREE_CASE_LIST, OnCaseListSelectionChange)
    ON_NOTIFY(NM_CLICK, IDC_TREE_CASE_LIST, OnCaseListClick)
    ON_NOTIFY(TVN_KEYDOWN, IDC_TREE_CASE_LIST, OnCaseListKeydown)
END_MESSAGE_MAP()


DuplicateChooserDlg::DuplicateChooserDlg(std::vector<DuplicateInfo>& case_duplicates, const size_t duplicate_index,
                                        const size_t number_duplicates, CWnd* pParent/* = nullptr*/)
    :   m_caseDuplicates(case_duplicates),
        CDialog(IDD, pParent)
{
    m_duplicateIndexText = FormatText("Duplicate %d / %d", static_cast<int>(duplicate_index), static_cast<int>(number_duplicates));
}


void DuplicateChooserDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_TREE_CASE_LIST, m_caseList);
    DDX_Control(pDX, IDC_CASE_CONTENTS, m_caseContentsHtml);
}


BOOL DuplicateChooserDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    WindowsUtf8::SetText(this, IDC_CASE_KEY, "Case Key: " + m_caseDuplicates.front().data_case->GetKey());
    WindowsUtf8::SetText(this, IDC_DUPLICATE_NUMBER, m_duplicateIndexText);

    // setup the tree control

    // to allow the default selections to show up this is for some reason necessary
    m_caseList.ModifyStyle(TVS_CHECKBOXES, 0);
    m_caseList.ModifyStyle(0, TVS_CHECKBOXES);

    TVINSERTSTRUCT tv_insert_struct;
    tv_insert_struct.item.mask = TVIF_PARAM | TVIF_TEXT;
    tv_insert_struct.hInsertAfter = 0;

    const IndexResult* previous_index_result = nullptr;
    HTREEITEM current_connection_string_node = nullptr;
    HTREEITEM first_case_node = nullptr;

    auto add_tree_node = [&](HTREEITEM parent_node, std::wstring text, LPARAM lParam) -> HTREEITEM
    {
        tv_insert_struct.hParent = parent_node;
        tv_insert_struct.item.pszText = text.data();
        tv_insert_struct.item.cchTextMax = int32_cast(text.length());
        tv_insert_struct.item.lParam = lParam;

        HTREEITEM node = m_caseList.InsertItem(&tv_insert_struct);
        m_caseList.SetCheck(node);

        return node;
    };

    for( size_t i = 0; i < m_caseDuplicates.size(); ++i )
    {
        const DuplicateInfo& duplicate = m_caseDuplicates[i];

        // add the connection string
        if( duplicate.index_result != previous_index_result )
        {
            current_connection_string_node = add_tree_node(TVI_ROOT, TC::ToWide(duplicate.index_result->repository_name), -1);
            previous_index_result = duplicate.index_result;
        }

        const std::string index_text = FormatText(( duplicate.line_number == 0 ) ? "Case #%d" : "Case #%d  |  Line #%d",
                                                  static_cast<int>(duplicate.case_index),
                                                  static_cast<int>(duplicate.line_number));

        HTREEITEM case_node = add_tree_node(current_connection_string_node, TC::ToWide(index_text), i);

        if( i == 0 )
            first_case_node = case_node;
    }

    // expand all the nodes
    for( HTREEITEM node = m_caseList.GetRootItem(); node != nullptr; node = m_caseList.GetNextSiblingItem(node) )
        m_caseList.Expand(node, TVE_EXPAND);

    m_caseList.SelectItem(first_case_node);

    m_caseContentsHtml.SetContextMenuEnabled(false);

    return TRUE;
}


void DuplicateChooserDlg::OnHelp()
{
    // the help button is added with a non-standard ID (IDC_DLG_HELP) as this seems necessary to get the
    // button to appear (probably because the main dialog has a menu)
    AfxGetApp()->HtmlHelp(HID_BASE_RESOURCE + IDD);
}


void DuplicateChooserDlg::OnOK()
{
    // update the keep values
    for( HTREEITEM parent_node = m_caseList.GetRootItem();
         parent_node != nullptr;
         parent_node = m_caseList.GetNextSiblingItem(parent_node) )
    {
        for( HTREEITEM case_node = m_caseList.GetChildItem(parent_node);
             case_node != nullptr;
             case_node = m_caseList.GetNextSiblingItem(case_node) )
        {
            if( !m_caseList.GetCheck(case_node) )
            {
                const LPARAM case_node_data = m_caseList.GetItemData(case_node);
                m_caseDuplicates[case_node_data].keep = false;
            }
        }
    }

    CDialog::OnOK();
}


void DuplicateChooserDlg::OnCaseListSelectionChange(NMHDR* /*pNMHDR*/, LRESULT* const pResult)
{
    std::optional<std::string> case_contents_html;

    const HTREEITEM selected_node = m_caseList.GetSelectedItem();

    if( selected_node != nullptr )
    {
        const LPARAM selected_data = m_caseList.GetItemData(selected_node);

        if( selected_data >= 0 )
        {
            const DuplicateInfo& duplicate = m_caseDuplicates[selected_data];
            case_contents_html = m_caseToHtmlConverter.ToHtml(*duplicate.data_case);
        }
    }

    m_caseContentsHtml.SetHtml(case_contents_html.has_value() ? *case_contents_html :
                                                                "<html></html>");

    *pResult = 0;
}


void DuplicateChooserDlg::ProcessCaseListClick()
{
    const HTREEITEM selected_node = m_caseList.GetSelectedItem();
    const LPARAM selected_data = m_caseList.GetItemData(selected_node);
    const bool new_state = !m_caseList.GetCheck(selected_node); // ! because it hasn't changed quite yet

    // when clicking on a file node, process all the children
    if( selected_data < 0 )
    {
        for( HTREEITEM node = m_caseList.GetChildItem(selected_node);
             node != nullptr;
             node = m_caseList.GetNextSiblingItem(node) )
        {
            m_caseList.SetCheck(node, new_state);
        }
    }

    // otherwise they clicked on a node, so see if the parent node should have a
    // binary full selection check (partial selections would be too much to work out)
    else
    {
        HTREEITEM parent_node = m_caseList.GetParentItem(selected_node);
        bool all_checked = new_state;

        for( HTREEITEM node = m_caseList.GetChildItem(parent_node);
             all_checked && node != nullptr;
             node = m_caseList.GetNextSiblingItem(node) )
        {
            if( node != selected_node )
                all_checked = m_caseList.GetCheck(node);
        }

        m_caseList.SetCheck(parent_node, all_checked);
    }
}


void DuplicateChooserDlg::OnCaseListClick(NMHDR* /*pNMHDR*/, LRESULT* const pResult)
{
    const DWORD dw = GetMessagePos();
    CPoint point = CPoint(MAKEPOINTS(dw).x, MAKEPOINTS(dw).y);
    m_caseList.ScreenToClient(&point);

    UINT uFlags;
    const HTREEITEM selected_node = m_caseList.HitTest(point, &uFlags);

    if( selected_node != nullptr && ( uFlags & TVHT_ONITEMSTATEICON ) != 0 )
    {
        m_caseList.SelectItem(selected_node);
        ProcessCaseListClick();
    }

    *pResult = 0;
}


void DuplicateChooserDlg::OnCaseListKeydown(NMHDR* const pNMHDR, LRESULT* const pResult)
{
    const TV_KEYDOWN* const keydown = reinterpret_cast<const TV_KEYDOWN*>(pNMHDR);

    if( keydown->wVKey == VK_SPACE )
        ProcessCaseListClick();

    *pResult = 0;
}
