#include "StdAfx.h"
#include "TableSpecPropertiesPageDlg.h"
#include "ManageFilesDlg.h"
#include <zTableO/Table.h>


BEGIN_MESSAGE_MAP(TableSpecPropertiesPageDlg, CDialog)
    ON_EN_CHANGE(IDC_NAME, OnNameChange)
END_MESSAGE_MAP()


unsigned int TableSpecPropertiesPageDlg::GetDialogTemplateId()
{
    return IDD_PAGE_PROPERTIES_TABLE_SPEC;
}


TableSpecPropertiesPageDlg::TableSpecPropertiesPageDlg(ManageFilesDlg& manage_files_dlg, std::shared_ptr<CTabSet> table_spec,
                                                       std::string table_spec_file_path, CWnd* const pParent/* = nullptr*/)
    :   CDialog(GetDialogTemplateId(), pParent),
        m_manageFilesDlg(manage_files_dlg),
        m_tableSpec(std::move(table_spec)),
        m_filePath(std::move(table_spec_file_path)),
        m_name(m_tableSpec->GetName()),
        m_label(m_tableSpec->GetLabel())
{
}


void TableSpecPropertiesPageDlg::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_NAME, m_name);
    DDX_Text(pDX, IDC_LABEL, m_label);
}


void TableSpecPropertiesPageDlg::OnNameChange()
{
    std::wstring new_name = WindowsWS::GetDlgItemText(this, IDC_NAME);

    if( m_name != new_name )
        m_manageFilesDlg.UpdateNameInTree(*this, std::move(new_name));
}


void TableSpecPropertiesPageDlg::OnValidatePage()
{
    UpdateData(TRUE);

    // validate the name and label
    m_manageFilesDlg.ValidateName(GetName(), GetFilePath());

    if( SO::IsWhitespace(m_label) )
        throw CSProException("The label for '%s' cannot be blank.", GetName().c_str());
}


bool TableSpecPropertiesPageDlg::ApplyChanges()
{
    std::string name = GetName();
    std::string label = TC::ToUtf8(m_label);

    if( name == UTF8_TODO::GetUtf8(m_tableSpec->GetName()) &&
        label == UTF8_TODO::GetUtf8(m_tableSpec->GetLabel()) )
    {
        return false;
    }

    m_tableSpec->SetName(UTF8_TODO::GetCString(std::move(name)));
    m_tableSpec->SetLabel(UTF8_TODO::GetCString(std::move(label)));

    return true;
}
