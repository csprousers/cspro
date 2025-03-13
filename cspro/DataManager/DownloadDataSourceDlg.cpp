#include "StdAfx.h"
#include "DownloadDataSourceDlg.h"
#include "SyncHelpers.h"
#include <zUtilO/DynamicLayoutControlResizer.h>


BEGIN_MESSAGE_MAP(DownloadDataSourceDlg, DynamicLayoutResizableDlg)
    ON_COMMAND(IDC_SHOW_AVAILABLE_DATA, OnShowAvailableData)
    ON_COMMAND(IDC_DATA_SOURCE_SELECT, OnSelectDataSource)
END_MESSAGE_MAP()


DownloadDataSourceDlg::DownloadDataSourceDlg(CWnd* const pParent/* = nullptr*/)
    :   DynamicLayoutResizableDlg(IDD_DOWNLOAD_DATA_SOURCE, pParent),
        m_syncServiceSelectorDlg(SyncConnectionString(), this)
{
    SerializeDialogSize("DownloadDataSourceDlg");
}


DownloadDataSourceDlg::~DownloadDataSourceDlg()
{
}


void DownloadDataSourceDlg::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_DATA_LIST, m_dataListCtrl);
    DDX_Text(pDX, IDC_DATA_SOURCE, m_connectionString);

    if( m_syncServiceSelectorDlg.GetSafeHwnd() != nullptr )
        m_syncServiceSelectorDlg.UpdateData(pDX->m_bSaveAndValidate);
}


BOOL DownloadDataSourceDlg::OnInitDialog()
{
    const BOOL result = __super::OnInitDialog();

    m_syncServiceSelectorDlg.Create(this, IDC_SYNC_SERVICE);

    SyncHelpers::SetUpDataListCtrl(m_dataListCtrl);

    return result;
}


std::vector<std::tuple<CWnd*, SizingDirection>> DownloadDataSourceDlg::GetDynamicLayoutControls()
{
    return { { &m_syncServiceSelectorDlg, SizingDirection::X } };
}


void DownloadDataSourceDlg::OnOK()
{
    UpdateData(TRUE);

    std::optional<SyncRunner> sync_runner;

    try
    {
        const SyncConnectionString sync_connection_string = m_syncServiceSelectorDlg.ValidateSyncConnectionString();

        if( m_dataListCtrl.GetSelectedCount() != 1 )
            throw CSProException("Select the data to download.");

        ASSERT(static_cast<size_t>(m_dataListCtrl.GetSelectionMark()) < m_lastQueriedDictionaries.size());

        m_syncTask = SyncHelpers::CreateSyncTaskForUnopenedDataRepositories(SyncHelpers::CreateSyncRunnerAndConnect(sync_runner, sync_connection_string),
                                                                            m_lastQueriedDictionaries[m_dataListCtrl.GetSelectionMark()].GetSyncableName(),
                                                                            m_connectionString,
                                                                            SyncDirection::Get,
                                                                            false);
        if( m_syncTask != nullptr )
            __super::OnOK();
    }

    catch( const CSProException& exception )
    {
        sync_runner.has_value() ? sync_runner->ReportError(exception) :
                                  ErrorMessage::Display(exception);
    }
}


void DownloadDataSourceDlg::OnShowAvailableData()
{
    UpdateData(TRUE);

    m_lastQueriedDictionaries.clear();
    m_dataListCtrl.DeleteAllItems();

    SyncRunner sync_runner = SyncHelpers::CreateSyncRunner();

    try
    {
        SyncRunner::Connection& sync_runner_connection = ConnectToSyncService(sync_runner, m_syncServiceSelectorDlg.ValidateSyncConnectionString());

        m_lastQueriedDictionaries = sync_runner.GetDictionaries(SyncHelpers::GetSyncService(sync_runner_connection));

        SyncHelpers::FillDataList(m_dataListCtrl, m_lastQueriedDictionaries);
    }

    catch( const CSProException& exception )
    {
        sync_runner.ReportError(exception);
    }

    GetDlgItem(IDOK)->EnableWindow(!m_lastQueriedDictionaries.empty());
}


SyncRunner::Connection& DownloadDataSourceDlg::ConnectToSyncService(SyncRunner& sync_runner, const SyncConnectionString& sync_connection_string)
{
    std::string sync_connection_string_text = sync_connection_string.ToString();
    auto lookup = m_syncRunnerConnections.find(sync_connection_string_text);

    if( lookup == m_syncRunnerConnections.cend() )
    {
        lookup = m_syncRunnerConnections.try_emplace(std::move(sync_connection_string_text),
                                                     sync_runner.Connect(sync_connection_string)).first;
    }

    return lookup->second;
}


void DownloadDataSourceDlg::OnSelectDataSource()
{
    UpdateData(TRUE);

    DataFileDlg data_file_dlg(DataFileDlg::Type::CreateNew, true, m_connectionString, this);

    if( data_file_dlg.DoModal() != IDOK )
        return;

    m_connectionString = data_file_dlg.GetConnectionString();

    UpdateData(FALSE);
}
