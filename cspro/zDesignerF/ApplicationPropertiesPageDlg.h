#pragma once

#include <zAppO/Application.h>
#include <zUToolO/TreePropertiesPageValidator.h>

class ManageFilesDlg;


class ApplicationPropertiesPageDlg : public CDialog, public TreePropertiesPageValidator
{
public:
    static unsigned int GetDialogTemplateId();

    ApplicationPropertiesPageDlg(ManageFilesDlg& manage_files_dlg, const Application& application, CWnd* pParent = nullptr);

    std::string GetName() const  { return TC::ToUtf8(m_name); }
    std::string GetLabel() const { return TC::ToUtf8(m_label); }

    void OnValidatePage() override;

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;

    void OnNameChange();

private:
    ManageFilesDlg& m_manageFilesDlg;
    std::string m_filePath;

    std::wstring m_name;
    std::wstring m_label;
};
