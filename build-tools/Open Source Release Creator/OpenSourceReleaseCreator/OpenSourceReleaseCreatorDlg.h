#pragma once

#include "Creator.h"
#include <zUtilO/ResizableDlg.h>
#include <zUtilO/SettingsDb.h>
#include <zUtilF/LoggingListBox.h>


class OpenSourceReleaseCreatorDlg : public ResizableDlg
{
public:
    OpenSourceReleaseCreatorDlg(CWnd* pParent = nullptr);
    ~OpenSourceReleaseCreatorDlg();

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    void OnTagChange();

    void OnOK() override;

private:
    static constexpr std::string_view OutputDirectoryKey_sv = "output-directory";
    SettingsDb m_settingsDb;

    std::unique_ptr<Creator> m_creator;
    std::vector<Git::Tag> m_tags;

    CComboBox m_tagsComboBox;
    std::string m_commit;
    std::string m_outputDirectory;

    LoggingListBox m_loggingListBox;
};
