#include "StdAfx.h"
#include "SynchronizeDlg.h"
#include "SyncHelpers.h"
#include <zUtilO/DynamicLayoutControlResizer.h>
#include <zDataO/SyncHistoryEntry.h>


SynchronizeDlg::SynchronizeDlg(std::shared_ptr<DataRepository> data_repository, CWnd* const pParent,
                               std::tuple<SyncConnectionString, SyncDirection, std::string> sync_params)
    :   DynamicLayoutResizableDlg(IDD_SYNCHRONIZE, pParent),
        m_dataRepository(std::move(data_repository)),
        m_syncDirection(std::get<1>(sync_params)),
        m_syncDirectionRadioEnumHelper({ SyncDirection::Put,
                                         SyncDirection::Get,
                                         SyncDirection::Both }),
        m_universe(std::move(std::get<2>(sync_params))),
        m_syncServiceSelectorDlg(std::move(std::get<0>(sync_params)), this)
{
    ASSERT(m_dataRepository != nullptr);

    SerializeDialogSize("SynchronizeDlg");
}


SynchronizeDlg::SynchronizeDlg(std::shared_ptr<DataRepository> data_repository, CWnd* const pParent/* = nullptr*/)
    :   SynchronizeDlg(data_repository, pParent, GetSyncParams(data_repository))
{
}


SynchronizeDlg::~SynchronizeDlg()
{
}


void SynchronizeDlg::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_CBIndex(pDX, IDC_COMBO_DIRECTION, m_syncDirectionRadioEnumHelper, m_syncDirection);
    DDX_Text(pDX, IDC_UNIVERSE, m_universe);

    if( m_syncServiceSelectorDlg.GetSafeHwnd() != nullptr )
        m_syncServiceSelectorDlg.UpdateData(pDX->m_bSaveAndValidate);
}


BOOL SynchronizeDlg::OnInitDialog()
{
    const BOOL result = __super::OnInitDialog();

    m_syncServiceSelectorDlg.Create(this, IDC_SYNC_SERVICE);

    return result;
}


std::vector<std::tuple<CWnd*, SizingDirection>> SynchronizeDlg::GetDynamicLayoutControls()
{
    return { { &m_syncServiceSelectorDlg, SizingDirection::X } };
}


std::tuple<SyncConnectionString, SyncDirection, std::string> SynchronizeDlg::GetSyncParams(std::shared_ptr<DataRepository> data_repository)
{
    ASSERT(data_repository != nullptr);

    ISyncableDataRepository* const syncable_repository = data_repository->GetSyncableDataRepository();
    ASSERT(syncable_repository != nullptr);

    constexpr size_t EntriesNeededForCalculation = 2;

    const std::vector<SyncHistoryEntry>& sync_history = syncable_repository->GetSyncHistory(
        DeviceId(),
        std::nullopt,
        std::nullopt,
        EntriesNeededForCalculation
    );

    // if there is no history, suggest putting to the last used sync service, which
    // SyncServiceSelectorDlg will do with an undefined sync connection string
    if( sync_history.empty() )
        return { SyncConnectionString(), SyncDirection::Put, std::string() };

    const SyncHistoryEntry& last_sync_history = sync_history.front();

    std::tuple<SyncConnectionString, SyncDirection, std::string> sync_params =
    {
        last_sync_history.GetDeviceName(),
        last_sync_history.GetDirection(),
        last_sync_history.GetUniverse()
    };

    // because BOTH is equivalent to a GET and then a PUT, determine if the last sync was likely
    // a BOTH (which will be assumed if the syncs happened within five minutes of each other)
    constexpr uint64_t CloseSyncSeconds = DateHelper::SecondsInMinute(5);

    if( std::get<1>(sync_params) == SyncDirection::Put && sync_history.size() > 1 )
    {
        const SyncHistoryEntry& second_to_last_sync_history = sync_history[1];

        if( second_to_last_sync_history.GetDirection() == SyncDirection::Get &&
            second_to_last_sync_history.GetDeviceName() == last_sync_history.GetDeviceName() &&
            second_to_last_sync_history.GetUniverse() == last_sync_history.GetUniverse() )
        {
            const int64_t seconds_different = last_sync_history.GetDateTime() - second_to_last_sync_history.GetDateTime();

            if( static_cast<uint64_t>(seconds_different) <= CloseSyncSeconds )
                std::get<1>(sync_params) = SyncDirection::Both;
        }
    }

    return sync_params;
}


void SynchronizeDlg::OnOK()
{
    UpdateData(TRUE);

    std::optional<SyncRunner> sync_runner;

    try
    {
        const SyncConnectionString sync_connection_string = m_syncServiceSelectorDlg.ValidateSyncConnectionString();

        if( !m_universe.empty() )
        {
            const CDataDict& dictionary = m_dataRepository->GetCaseAccess().GetDataDict();
            const size_t key_length = dictionary.GetKeyLength();
            const size_t universe_length = SO::WideLength(m_universe);

            if( universe_length > key_length )
            {
                throw CSProException("You cannot sync with a universe longer than the key length of '%s' (%d > %d).",
                                     dictionary.GetName().c_str(), static_cast<int>(universe_length), static_cast<int>(key_length));
            }
        }

        m_syncTask = std::make_unique<SyncTask>(SyncHelpers::CreateSyncRunnerAndConnect(sync_runner, sync_connection_string),
                                                m_dataRepository, m_syncDirection, m_universe);

        __super::OnOK();
    }

    catch( const CSProException& exception )
    {
        sync_runner.has_value() ? sync_runner->ReportError(exception) :
                                  ErrorMessage::Display(exception);
    }
}
