#include "StdAfx.h"
#include "ProductionSyncer.h"
#include "SyncHelpers.h"
#include "SyncTask.h"
#include "TaskRunnerDlg.h"
#include <zNetwork/SyncException.h>


ProductionSyncer::ProductionSyncer(std::unique_ptr<const PFF> pff)
    :   m_pff(std::move(pff))
{
    ASSERT(m_pff != nullptr);
}


std::unique_ptr<ProductionSyncer> ProductionSyncer::Create(const std::vector<std::wstring>& file_paths)
{
    const auto& lookup = std::find_if(file_paths.cbegin(), file_paths.cend(),
        [&](const std::wstring& file_path)
        {
            return SO::EqualsNoCase(Path::GetExtension(TC::ToUtf8(file_path)), FileExtensions::Pff);
        });

    if( lookup == file_paths.cend() )
        return nullptr;

    if( file_paths.size() > 1 )
        throw CSProException("When processing a PFF, you can only specify one file on the command line.");

    auto pff = std::make_unique<PFF>(WS2CS(file_paths.front()));

    if( !pff->LoadPifFile() || pff->GetAppType() != APPTYPE::Sync )
    {
        throw CSProException("PFF file '%s' was not read correctly. Check the file for parameters invalid to Data Manager.",
                             Path::GetFilename(UTF8_TODO::GetUtf8(pff->GetPifFileName())).c_str());
    }

    return std::unique_ptr<ProductionSyncer>(new ProductionSyncer(std::move(pff)));
}


void ProductionSyncer::RunSyncs()
{
    std::optional<SyncRunner> sync_runner;

    auto handle_exception = [&](const CSProException& exception)
    {
        // rethrow user or sync cancelations
        if( dynamic_cast<const UserCanceledException*>(&exception) != nullptr ||
            dynamic_cast<const SyncCancelException*>(&exception) != nullptr )
        {
            throw UserCanceledException();
        }

        sync_runner.has_value() ? sync_runner->ReportError(exception) :
                                  ErrorMessage::Display(exception);
    };

    try
    {
        // ensure that at least one dictionary is being synced
        if( m_pff->GetExternalDataConnectionStrings().empty() )
        {
            throw CSProException("PFF file '%s' does the contain the name of a dictionary to sync.",
                                 Path::GetFilename(UTF8_TODO::GetUtf8(m_pff->GetPifFileName())).c_str());
        }

        // validate and connect to the sync service
        m_pff->GetSyncService().Validate();

        SyncRunner::Connection sync_runner_connection = SyncHelpers::CreateSyncRunnerAndConnect(sync_runner, m_pff->GetSyncService());

        // sync each dictionary
        for( const auto& [dictionary_name, connection_string] : m_pff->GetExternalDataConnectionStrings() )
        {
            try
            {
                RunSync(sync_runner_connection, UTF8_TODO::GetUtf8(dictionary_name), connection_string);
            }

            catch( const CSProException& exception )
            {
                handle_exception(exception);
            }
        }
    }

    catch( const CSProException& exception )
    {
        handle_exception(exception);
    }
}


void ProductionSyncer::RunSync(SyncRunner::Connection sync_runner_connection, const std::string& dictionary_name, const ConnectionString& connection_string)
{
    // create the sync task
    std::unique_ptr<SyncTask> sync_task = SyncHelpers::CreateSyncTaskForUnopenedDataRepositories(std::move(sync_runner_connection),
                                                                                                 dictionary_name,
                                                                                                 connection_string,
                                                                                                 m_pff->GetSyncDirection(),
                                                                                                 true);
    if( sync_task == nullptr )
        return;

    // run the sync
    TaskRunnerDlg task_runner_dlg(std::move(sync_task));
    task_runner_dlg.SetCloseDialogOnSuccess();

    switch( task_runner_dlg.DoModal() )
    {
        case IDOK:
            m_connectionStringsForSyncedDataSources.emplace_back(connection_string);
            break;

        case IDCANCEL:
            throw UserCanceledException();
    }
}
