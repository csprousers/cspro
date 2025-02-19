#pragma once

#include <zFormO/FormFile.h>
#include <zUToolO/TreePropertiesPageValidator.h>

class ManageFilesDlg;


class FormFilePropertiesPageDlg : public CDialog, public TreePropertiesPageValidator
{
public:
    static unsigned int GetDialogTemplateId();

    FormFilePropertiesPageDlg(ManageFilesDlg& manage_files_dlg,
                              std::variant<std::shared_ptr<CDEFormFile>, std::shared_ptr<const CDEFormFile>> form_file,
                              std::string form_file_path,
                              CWnd* pParent = nullptr);

    const std::string& GetFilePath() const { return m_filePath; }

    std::string GetName() const { return TC::ToUtf8(m_name); }

    void OnValidatePage() override;

    // called following validation in ManageFilesDlg::OnOK, this returns true if the form file properties changed
    bool ApplyChanges();

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    void OnNameChange();

private:
    bool IsFormFileModifiable() const;

private:
    ManageFilesDlg& m_manageFilesDlg;
    std::variant<std::shared_ptr<CDEFormFile>, std::shared_ptr<const CDEFormFile>> m_formFile;
    const CDEFormFile* m_formFilePtr;
    std::string m_filePath;

    std::wstring m_name;
    std::wstring m_label;
};
