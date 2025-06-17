#pragma once

#include <zUtilO/ResizableDlg.h>
#include <zUtilO/SyncConnectionString.h>
#include <zUtilF/SortListCtrl.h>
#include <zSyncO/SyncDictionaryInfo.h>

class CSWebConnection;
class SyncRunner;


class OpenCSWebDataDlg : public ResizableDlg
{
public:
    OpenCSWebDataDlg(CWnd* pParent = nullptr);
    ~OpenCSWebDataDlg();

    ConnectionString GetConnectionString() const;

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    void OnOK() override;

    void OnShowAvailableData();

private:
    CSWebConnection& ConnectToCSWeb(SyncRunner& sync_runner);

private:
    SyncConnectionString m_syncConnectionString;
    CSortListCtrl m_dataListCtrl;

    std::map<std::string, std::unique_ptr<CSWebConnection>> m_cswebConnections;
    std::vector<SyncDictionaryInfo> m_lastQueriedDictionaries;
    size_t m_selectedDictionaryIndex;
};
