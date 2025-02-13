#include "StdAfx.h"
#include "RenDlg.h"


BEGIN_MESSAGE_MAP(CRenameDlg, CDialog)
    ON_BN_CLICKED(IDC_RENAME, OnRename)
    ON_BN_CLICKED(IDC_DELETE, OnDelete)
END_MESSAGE_MAP()


CRenameDlg::CRenameDlg(const CDEFormBase& form_base, const std::vector<const DictNamedBase*>& dict_candidates, CWnd* pParent/* = nullptr*/)
    :   CDialog(CRenameDlg::IDD, pParent),
        m_formBase(form_base),
        m_dictCandidates(dict_candidates),
        m_renameOrDelete(1),
        m_deleteAllItemsNotFound(FALSE),
        m_selectedDictCandidate(nullptr)
{
    ASSERT(!m_dictCandidates.empty());
}


void CRenameDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_NFMSG, m_heading);
    DDX_Control(pDX, IDC_ITEMSEL, m_candidateList);
    DDX_Radio(pDX, IDC_RENAME, m_renameOrDelete);
    DDX_Check(pDX, IDC_DELETEALL, m_deleteAllItemsNotFound);
}


BOOL CRenameDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    CString name = m_formBase.GetName();

    if( m_formBase.IsKindOf(RUNTIME_CLASS(CDEGroup)) )
        name = assert_cast<const CDEGroup&>(m_formBase).GetTypeName();

    m_heading = name + _T(" not found in dictionary");

    SetDlgItemText(IDC_RENAME, FormatText(_T("&Rename %s to"), (LPCTSTR)name));
    SetDlgItemText(IDC_DELETE, FormatText(_T("&Delete %s from the form"), (LPCTSTR)name));

    for( const DictNamedBase* dict_candidate : m_dictCandidates )
        m_candidateList.AddString(dict_candidate->GetName());

    m_candidateList.EnableWindow(FALSE);

    UpdateData(FALSE);

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}


void CRenameDlg::OnOK()
{
    UpdateData(TRUE);

    if( m_renameOrDelete == 0 )
    {
        int index = m_candidateList.GetCurSel();

        if( index == LB_ERR )
        {
            AfxMessageBox(_T("Please select a new item"));
            return;
        }

        m_selectedDictCandidate = m_dictCandidates[index];
    }

    CDialog::OnOK();
}


void CRenameDlg::OnRename()
{
    m_candidateList.EnableWindow(TRUE);
    m_candidateList.ShowDropDown(TRUE);
}


void CRenameDlg::OnDelete()
{
    UpdateData(TRUE);
    m_candidateList.SetCurSel(-1);
    UpdateData(FALSE);
    m_candidateList.EnableWindow(FALSE);
}
