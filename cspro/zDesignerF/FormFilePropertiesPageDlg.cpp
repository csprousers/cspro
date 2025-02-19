#include "StdAfx.h"
#include "FormFilePropertiesPageDlg.h"
#include "ManageFilesDlg.h"


BEGIN_MESSAGE_MAP(FormFilePropertiesPageDlg, CDialog)
    ON_EN_CHANGE(IDC_NAME, OnNameChange)
END_MESSAGE_MAP()


unsigned int FormFilePropertiesPageDlg::GetDialogTemplateId()
{
    return IDD_PAGE_PROPERTIES_FORM_FILE;
}


FormFilePropertiesPageDlg::FormFilePropertiesPageDlg(ManageFilesDlg& manage_files_dlg,
                                                    std::variant<std::shared_ptr<CDEFormFile>, std::shared_ptr<const CDEFormFile>> form_file,
                                                    std::string form_file_path,
                                                    CWnd* const pParent/* = nullptr*/)
    :   CDialog(GetDialogTemplateId(), pParent),
        m_manageFilesDlg(manage_files_dlg),
        m_formFile(std::move(form_file)),
        m_formFilePtr(std::visit([](const auto& form_file) -> const CDEFormFile* { return form_file.get(); }, m_formFile)),
        m_filePath(std::move(form_file_path)),
        m_name(m_formFilePtr->GetName()),
        m_label(m_formFilePtr->GetLabel())
{
}


bool FormFilePropertiesPageDlg::IsFormFileModifiable() const
{
    return std::holds_alternative<std::shared_ptr<CDEFormFile>>(m_formFile);
}


void FormFilePropertiesPageDlg::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_NAME, m_name);
    DDX_Text(pDX, IDC_LABEL, m_label);
}


BOOL FormFilePropertiesPageDlg::OnInitDialog()
{
    const BOOL result = __super::OnInitDialog();

    if( !IsFormFileModifiable() )
    {
        GetDlgItem(IDC_NAME)->EnableWindow(FALSE);
        GetDlgItem(IDC_LABEL)->EnableWindow(FALSE);
    }

    return result;
}


void FormFilePropertiesPageDlg::OnNameChange()
{
    ASSERT(IsFormFileModifiable());

    std::wstring new_name = WindowsWS::GetDlgItemText(this, IDC_NAME);

    if( m_name != new_name )
        m_manageFilesDlg.UpdateNameInTree(*this, std::move(new_name));
}


void FormFilePropertiesPageDlg::OnValidatePage()
{
    if( !IsFormFileModifiable() )
        return;

    UpdateData(TRUE);

    // validate the name and label
    m_manageFilesDlg.ValidateName(GetName(), GetFilePath());

    if( SO::IsWhitespace(m_label) )
        throw CSProException("The label for '%s' cannot be blank.", GetName().c_str());
}


bool FormFilePropertiesPageDlg::ApplyChanges()
{
    if( !IsFormFileModifiable() )
        return false;

    CDEFormFile& modifiable_form_file = *std::get<std::shared_ptr<CDEFormFile>>(m_formFile);

    std::string name = GetName();
    std::string label = TC::ToUtf8(m_label);

    const bool name_changed = ( name != UTF8_TODO::GetUtf8(modifiable_form_file.GetName()) );

    if( !name_changed && label == UTF8_TODO::GetUtf8(modifiable_form_file.GetLabel()) )
        return false;

    if( name_changed )
    {
        modifiable_form_file.RemoveUniqueName(modifiable_form_file.GetName());
        modifiable_form_file.SetName(UTF8_TODO::GetCString(std::move(name)));
        modifiable_form_file.BuildUniqueNL();
    }

    modifiable_form_file.SetLabel(UTF8_TODO::GetCString(std::move(label)));

    return true;
}
