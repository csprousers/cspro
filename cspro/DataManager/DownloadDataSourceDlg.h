#pragma once

#include <DataManager/SyncTask.h>
#include <zUtilO/ResizableDlg.h>
#include <zUtilF/SrtLstCt.h>
#include <zSyncO/SyncDictionaryInfo.h>
#include <zSyncF/SyncServiceSelectorDlg.h>


class DownloadDataSourceDlg : public DynamicLayoutResizableDlg
{
public:
    DownloadDataSourceDlg(CWnd* pParent = nullptr);
    ~DownloadDataSourceDlg();

    std::unique_ptr<SyncTask> ReleaseSyncTask() { return std::move(m_syncTask); }

    const ConnectionString& GetConnectionString() const { return m_connectionString; }

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    std::vector<std::tuple<CWnd*, SizingDirection>> GetDynamicLayoutControls() override;

    void OnOK() override;

    void OnShowAvailableData();

    void OnSelectDataSource();

private:
    SyncRunner::Connection& ConnectToSyncService(SyncRunner& sync_runner, const SyncConnectionString& sync_connection_string);

private:
    SyncServiceSelectorDlg m_syncServiceSelectorDlg;
    CSortListCtrl m_dataListCtrl;
    ConnectionString m_connectionString;

    std::map<std::string, SyncRunner::Connection> m_syncRunnerConnections;
    std::vector<SyncDictionaryInfo> m_lastQueriedDictionaries;

    std::unique_ptr<SyncTask> m_syncTask;
};
