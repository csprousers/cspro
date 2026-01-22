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
    bool InitializeOperation() noexcept;

    void EnableButtons(bool enable);

    void OnCreateValidate(bool create);

    void CreateValidateWorker(bool create);
    LRESULT OnCreateValidateComplete(WPARAM wParam, LPARAM lParam);

    LRESULT OnDisplayErrorMessage(WPARAM wParam, LPARAM lParam);

private:
    SettingsDb m_settingsDb;

    std::string m_openSourceDirectory;
    LoggingListBox m_loggingListBox;
    CComboBox m_tagsComboBox;
    std::string m_commit;

    std::unique_ptr<Syncer> m_syncer;
    std::vector<GitTag> m_tags;
    std::unique_ptr<std::thread> m_workerThread;
};
