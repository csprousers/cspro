#pragma once

#include <zUtilO/ResizableDlg.h>
#include <zUtilO/TemporaryFile.h>


class PropertiesDlg : public ResizableDlg
{
public:
    PropertiesDlg(SettingsDb& global_settings_db, CWnd* pParent = nullptr);

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;

    void OnOK() override;

    void OnDiffToolSelect();
    void OnDiffToolTest();

private:
    SettingsDb& m_globalSettingsDb;
    std::string m_diffToolCommand;
    std::string m_diffToolArgument;

    std::unique_ptr<std::tuple<TemporaryFile, TemporaryFile>> m_diffToolTextFiles;
};
