#include "StdAfx.h"
#include "ReportPropertiesPageDlg.h"
#include "ManageFilesDlg.h"


BEGIN_MESSAGE_MAP(ReportPropertiesPageDlg, CDialog)
    ON_EN_CHANGE(IDC_NAME, OnNameChange)
END_MESSAGE_MAP()


unsigned int ReportPropertiesPageDlg::GetDialogTemplateId()
{
    return IDD_PAGE_PROPERTIES_REPORT;
}


ReportPropertiesPageDlg::ReportPropertiesPageDlg(ManageFilesDlg& manage_files_dlg, ReportFile report_file, CWnd* const pParent/* = nullptr*/)
    :   CDialog(GetDialogTemplateId(), pParent),
        m_manageFilesDlg(manage_files_dlg),
        m_reportFile(std::move(report_file)),
        m_name(TC::ToWide(m_reportFile.GetName())),
        m_escapeTypeRadioEnumHelper({ ReportFile::EscapeType::Html,
                                      ReportFile::EscapeType::Markdown,
                                      ReportFile::EscapeType::Csv,
                                      ReportFile::EscapeType::None }),
        m_escapeType(m_escapeTypeRadioEnumHelper.ToForm(m_reportFile.GetEscapeType()))
{
}


void ReportPropertiesPageDlg::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_NAME, m_name);
    DDX_Radio(pDX, IDC_ESCAPE_HTML, m_escapeType);
}


void ReportPropertiesPageDlg::OnNameChange()
{
    std::wstring new_name = WindowsWS::GetDlgItemText(this, IDC_NAME);

    if( m_name != new_name )
        m_manageFilesDlg.UpdateNameInTree(*this, std::move(new_name));
}


void ReportPropertiesPageDlg::OnValidatePage()
{
    UpdateData(TRUE);

    // validate the name
    std::string utf8_name = TC::ToUtf8(m_name);
    m_manageFilesDlg.ValidateName(utf8_name, m_reportFile.GetFilePath());

    m_reportFile.SetName(std::move(utf8_name));
    m_reportFile.SetEscapeType(m_escapeTypeRadioEnumHelper.FromForm(m_escapeType));
}
