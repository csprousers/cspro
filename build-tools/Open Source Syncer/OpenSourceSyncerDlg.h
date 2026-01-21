#pragma once

#include "Syncer.h"
#include <zUtilO/ResizableDlg.h>
#include <zGit/GitTag.h>


class OpenSourceSyncerDlg : public ResizableDlg
{
public:
    OpenSourceSyncerDlg(CWnd* pParent = nullptr);
    ~OpenSourceSyncerDlg();

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    void OnCancel() override;

    void OnTagChange();

    void OnCreate()   { OnCreateValidate(true); }
    void OnValidate() { OnCreateValidate(false); }

    void OnGenerateFileList();

private:
    void EnableButtons(bool enable);

    void OnCreateValidate(bool create);

    void CreateValidateWorker(bool create);
    LRESULT OnCreateValidateComplete(WPARAM wParam, LPARAM lParam);

    LRESULT OnDisplayErrorMessage(WPARAM wParam, LPARAM lParam);

private:
    static constexpr std::string_view OutputDirectoryKey_sv = "output-directory";
    SettingsDb m_settingsDb;

    std::unique_ptr<Syncer> m_syncer;
    std::vector<GitTag> m_tags;

    CComboBox m_tagsComboBox;
    std::string m_commit;
    std::string m_outputDirectory;

    LoggingListBox m_loggingListBox;

    std::unique_ptr<std::thread> m_workerThread;
};
