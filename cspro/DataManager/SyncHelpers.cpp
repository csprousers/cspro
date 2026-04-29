#include "StdAfx.h"
#include "SyncHelpers.h"
#include "SyncTask.h"
#include <zUtilF/SortListCtrl.h>
#include <zDataO/DataRepositoryHelpers.h>
#include <zSyncO/DialogBasedSyncListener.h>
#include <zSyncO/SyncDictionaryInfo.h>


void SyncHelpers::SetUpDataListCtrl(CSortListCtrl& data_list_ctrl)
{
    data_list_ctrl.SetExtendedStyle(LVS_EX_FULLROWSELECT);
    data_list_ctrl.SetHeadings(L"Label,200;Name,150;Cases (Not Deleted),130");
    data_list_ctrl.LoadColumnInfo();
}


void SyncHelpers::FillDataList(CSortListCtrl& data_list_ctrl, std::vector<SyncDictionaryInfo>& queried_dictionaries)
{
    ASSERT(data_list_ctrl.GetItemCount() == 0);

    if( queried_dictionaries.empty() )
    {
        ErrorMessage::Display(L"There is no data on the sync service. Upload a data file to the service or use a different service and try again.");
        return;
    }

    // show the dictionaries sorted by label
    std::sort(queried_dictionaries.begin(), queried_dictionaries.end(),
        [](const SyncDictionaryInfo& sdi1, const SyncDictionaryInfo& sdi2)
        {
            return ( sdi1.GetLabel() < sdi2.GetLabel() );
        });

    for( const SyncDictionaryInfo& sync_dictionary_info : queried_dictionaries )
    {
        // show the label, display name, and case counts (when present)
        data_list_ctrl.AddItem(TC::ToWide(sync_dictionary_info.GetLabel()).c_str(),
                               TC::ToWide(sync_dictionary_info.GetDisplayName()).c_str(),
                               ( sync_dictionary_info.GetCaseCount() >= 0 ) ? TC::ToWide(IntToString(sync_dictionary_info.GetCaseCount())).c_str() :
                                                                              L"<unknown>");
    }
}


void SyncHelpers::ValidateDataRepositoryTypeValidForSync(const ConnectionString& connection_string)
{
    if( !DataRepositoryHelpers::TypeSupportsSync(connection_string.GetType()) )
    {
        throw CSProException("Data sources of type '%s' cannot be used for synchronization. It is recommended that you sync to a CSPro DB file.",
                             ToString(connection_string.GetType()));
    }
}


SyncRunner SyncHelpers::CreateSyncRunner()
{
    return SyncRunner(std::make_unique<DialogBasedSyncListener>(nullptr));
}


SyncRunner::Connection SyncHelpers::CreateSyncRunnerAndConnect(std::optional<SyncRunner>& sync_runner, const SyncConnectionString& sync_connection_string)
{
    ASSERT(!sync_runner.has_value());
    sync_runner.emplace(CreateSyncRunner());
    return sync_runner->Connect(sync_connection_string);
}


ISyncService& SyncHelpers::GetSyncService(SyncRunner::Connection& sync_runner_connection)
{
    ASSERT(std::get<0>(sync_runner_connection) != nullptr);
    return *std::get<0>(sync_runner_connection);
}


std::unique_ptr<const CDataDict> SyncHelpers::GetDictionary(SyncRunner& sync_runner, SyncRunner::Connection& sync_runner_connection,
                                                            const std::string& dictionary_name)
{
    try
    {
        const std::string dictionary_spec = sync_runner.GetDictionarySpec(GetSyncService(sync_runner_connection), dictionary_name);

        auto dictionary = std::make_unique<CDataDict>();
        dictionary->OpenFromText(dictionary_spec);
        return dictionary;
    }

    catch( const CSProException& exception )
    {
        throw CSProException("The dictionary '%s' does not exist on the sync service or cannot be read by CSPro: %s",
                             dictionary_name.c_str(), exception.what());
    }
}


std::unique_ptr<SyncTask> SyncHelpers::CreateSyncTaskForUnopenedDataRepositories(SyncRunner::Connection sync_runner_connection,
                                                                                 const std::string& dictionary_name,
                                                                                 const ConnectionString& connection_string,
                                                                                 const SyncDirection sync_direction,
                                                                                 const bool production_sync_mode)

{
    if( !connection_string.IsDefined() )
        throw CSProException("You must specify a data source.");

    ValidateDataRepositoryTypeValidForSync(connection_string);

    SyncRunner sync_runner(std::make_unique<DialogBasedSyncListener>(nullptr));

    try
    {
        // download the dictionary
        std::unique_ptr<const CDataDict> dictionary = GetDictionary(sync_runner, sync_runner_connection, dictionary_name);

        // open or create the data source
        std::unique_ptr<DataRepository> data_repository =
            DataRepository::CreateAndOpen(CaseAccess::CreateAndInitializeFullCaseAccess(*dictionary),
                                          connection_string,
                                          DataRepositoryAccess::ReadWrite,
                                          production_sync_mode ? DataRepositoryOpenFlag::OpenOrCreate : DataRepositoryOpenFlag::CreateNew);

        // create the sync task
        return std::make_unique<SyncTaskForUnopenedDataRepositories>(std::move(sync_runner_connection),
                                                                     std::move(data_repository),
                                                                     sync_direction,
                                                                     !production_sync_mode,
                                                                     std::move(dictionary));
    }

    catch( const CSProException& exception )
    {
        sync_runner.ReportError(exception);
        return nullptr;
    }
}
