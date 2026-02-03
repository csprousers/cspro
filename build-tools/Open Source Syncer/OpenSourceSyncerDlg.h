#pragma once

#include "Syncer.h"
#include <zUtilO/ResizableDlg.h>


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

private:
    bool InitializeOperation() noexcept;

    LRESULT OnOperationComplete(WPARAM wParam, LPARAM lParam);

    LRESULT OnDisplayErrorMessage(WPARAM wParam, LPARAM lParam);

private:
    SettingsDb m_settingsDb;

    std::string m_openSourceDirectory;
    LoggingListBox m_loggingListBox;

    std::unique_ptr<Syncer> m_syncer;

    std::unique_ptr<std::thread> m_workerThread;
};
