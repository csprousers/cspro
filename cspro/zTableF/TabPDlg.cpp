#include "StdAfx.h"
#include "TabPDlg.h"
#include "TabDoc.h"
#include "TabTrCtl.h"


CTabPropDlg::CTabPropDlg(CTabSet* pTabSet, CTabTreeCtrl* pParent)
    :   CDialog(IDD_TABLE_PROP_DLG, pParent),
        m_pTabSet(pTabSet)
{
    const TableElementTreeNode* table_element_tree_node = pParent->GetTreeNode(pParent->GetSelectedItem());

    m_pTabDoc = table_element_tree_node->GetTabDoc();
    m_pTable = table_element_tree_node->GetTable();
    m_sTabName = m_pTable->GetName();
}


void CTabPropDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_FF_NAME, m_sTabName);
}


void CTabPropDlg::OnOK()
{
    UpdateData(TRUE);

    if(m_sTabName.IsEmpty())
    {
        AfxMessageBox(_T("Name cannot be empty."));
        GetDlgItem(IDC_FF_NAME)->SetFocus();
        return;
    }

    if(!m_sTabName.IsName())
    {
        AfxMessageBox(_T("Not a Valid Name."));
        return;
    }

    if(m_sTabName.IsReservedWord())
    {
        AfxMessageBox(FormatText(_T("%s is a reserved word."), m_sTabName.GetString()));
        return;
    }

    if ((m_pTable->GetName().CompareNoCase(m_sTabName) != 0) && m_pTabDoc)
    {
        if( !AfxGetMainWnd()->SendMessage(UWM::Table::IsNameUnique, reinterpret_cast<WPARAM>(m_sTabName.GetString()), reinterpret_cast<LPARAM>(m_pTabDoc)) )
            return;
    }

    CDialog::OnOK();
}
