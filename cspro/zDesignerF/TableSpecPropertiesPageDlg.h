#pragma once

#include <zUToolO/TreePropertiesPageValidator.h>

class CTabSet;
class ManageFilesDlg;


class TableSpecPropertiesPageDlg : public CDialog, public TreePropertiesPageValidator
{
public:
    static unsigned int GetDialogTemplateId();

    TableSpecPropertiesPageDlg(ManageFilesDlg& manage_files_dlg, std::shared_ptr<CTabSet> table_spec, std::string table_spec_file_path, CWnd* pParent = nullptr);

    const std::string& GetFilePath() const { return m_filePath; }

    std::string GetName() const { return TC::ToUtf8(m_name); }

    void OnValidatePage() override;

    // called following validation in ManageFilesDlg::OnOK, this returns true if the table spec's properties changed
    bool ApplyChanges();

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;

    void OnNameChange();

private:
    ManageFilesDlg& m_manageFilesDlg;
    std::shared_ptr<CTabSet> m_tableSpec;
    std::string m_filePath;

    std::wstring m_name;
    std::wstring m_label;
};
