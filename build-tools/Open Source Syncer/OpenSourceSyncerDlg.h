#pragma once

#include <zUtilO/ResizableDlg.h>

class Syncer;


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

    void OnSync();
    void OnCompare();

    void OnRefreshLibraryTags();
    void OnCommitLibrary();

    LRESULT OnOperationComplete(WPARAM wParam, LPARAM lParam);

    LRESULT OnDisplayErrorMessage(WPARAM wParam, LPARAM lParam);

private:
    // The operation callback will be run in a worker thread.
    void RunOperation(const std::function<void()>& validate_inputs_callback,
                      std::function<void()> operation_callback) noexcept;

    struct SyncData;
    void ValidateSyncData(SyncData& sync_data, bool using_oldest_merge_commit);

    struct LibraryData;
    void ValidateLibraryData(LibraryData& library_data, bool creating_commit);

private:
    SettingsDb m_settingsDb;

    std::string m_openSourceCodeDirectory;
    std::string m_openSourceLibrariesDirectory;
    std::string m_branchName;
    std::string m_commitOld;
    std::string m_commitNew;
    LoggingListBox m_loggingListBox;

    std::unique_ptr<Syncer> m_syncer;

    std::unique_ptr<std::thread> m_workerThread;
};
