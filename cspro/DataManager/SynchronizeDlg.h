#pragma once

#include <DataManager/SyncTask.h>
#include <zUtilO/ResizableDlg.h>
#include <zUtilF/DialogValidators.h>
#include <zSyncF/SyncServiceSelectorDlg.h>


class SynchronizeDlg : public DynamicLayoutResizableDlg
{
private:
    SynchronizeDlg(std::shared_ptr<DataRepository> data_repository, CWnd* pParent,
                   std::tuple<SyncConnectionString, SyncDirection, std::string> sync_params);

public:
    SynchronizeDlg(std::shared_ptr<DataRepository> data_repository, CWnd* pParent = nullptr);
    ~SynchronizeDlg();

    std::unique_ptr<SyncTask> ReleaseSyncTask() { return std::move(m_syncTask); }

protected:
    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    std::vector<std::tuple<CWnd*, SizingDirection>> GetDynamicLayoutControls() override;

    void OnOK() override;

private:
    static std::tuple<SyncConnectionString, SyncDirection, std::string> GetSyncParams(std::shared_ptr<DataRepository> data_repository);

private:
    std::shared_ptr<DataRepository> m_dataRepository;
    SyncDirection m_syncDirection;
    RadioEnumHelper<SyncDirection> m_syncDirectionRadioEnumHelper;
    std::string m_universe;

    SyncServiceSelectorDlg m_syncServiceSelectorDlg;

    std::unique_ptr<SyncTask> m_syncTask;
};
