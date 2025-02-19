#pragma once

#include <zAppO/ReportFile.h>
#include <zUToolO/TreePropertiesPageValidator.h>

class ManageFilesDlg;


class ReportPropertiesPageDlg : public CDialog, public TreePropertiesPageValidator
{
public:
    static unsigned int GetDialogTemplateId();

    ReportPropertiesPageDlg(ManageFilesDlg& manage_files_dlg, ReportFile report_file, CWnd* pParent = nullptr);

    const ReportFile& GetReportFile() const { return m_reportFile; }

    void OnValidatePage() override;

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;

    void OnNameChange();

private:
    ManageFilesDlg& m_manageFilesDlg;
    ReportFile m_reportFile;

    std::wstring m_name;

    RadioEnumHelper<ReportFile::EscapeType> m_escapeTypeRadioEnumHelper;
    int m_escapeType;
};
