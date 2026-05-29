#include "stdafx.h"
#include "DataSyncer.h"


#ifdef REFERENCE

const DeviceId& SyncClient::GetServerDeviceId() const
{
    ASSERT(m_connectResponse != nullptr);
    return m_connectResponse->GetServerDeviceId();
}


SyncClient::SyncResult SyncClient::SyncData(ISyncableDataRepository& repository, const SyncDirection direction, const std::string& universe)
{
    try
    {
        if( m_syncService == nullptr )
            throw SyncError(100132);

        const std::string repository_name = repository.GetName(DataRepositoryNameType::Concise);

        const SyncListenerServerSaverAndCloser sync_listener_saver_and_closer(m_syncService.get(), m_syncListener, 100104, repository_name.c_str());

        SYNCLOG_INFO << "Syncing data: " << repository_name
                     << " direction \"" << ToString(direction) << "\""
                     << " universe \"" << universe << "\"";

        if( m_syncService == nullptr )
            return SyncResult::SYNC_ERROR;

        if( direction == SyncDirection::Get || direction == SyncDirection::Both )
            SyncDataGet(repository, universe);

        if( direction == SyncDirection::Put || direction == SyncDirection::Both )
            SyncDataPut(repository, universe);

        return SyncResult::SYNC_OK;
    }

    catch( const SyncError& exception )
    {
        if( m_syncListener != nullptr )
            m_syncListener->ReportError(exception);

        SYNCLOG_ERROR << "Error syncing data: " << exception.GetErrorMessageNumber() << " " << exception.what();
        return SyncResult::SYNC_ERROR;
    }

    catch( const SyncCancelException& )
    {
        SYNCLOG_INFO << "Canceled sync data";
        return SyncResult::SYNC_CANCELED;
    }
}


void SyncClient::SyncDataGet(ISyncableDataRepository& repository, const std::string& universe)
{
    try
    {
        std::optional<SyncHistoryEntry> last_sync_revision = GetRevisionFromLastSync(SyncDirection::Get, repository, universe);

        if( last_sync_revision.has_value() )
        {
            SYNCLOG_INFO << "Last GET with this sync service " << DateTime::LocalDateTimeString(last_sync_revision->GetDateTime())
                         << " local revision " << last_sync_revision->GetFileRevision()
                         << " sync service revision \"" << last_sync_revision->GetServerFileRevision() << "\"";

            if( last_sync_revision->IsPartialGet() )
            {
                SYNCLOG_INFO << "Partial get, resume from case " << last_sync_revision->GetLastCaseUuid();

                if( last_sync_revision->GetUniverse() != universe )
                {
                    // Invalid to try to resume a sync with different params
                    SYNCLOG_INFO << "Universe doesn't match universe from partial - doing full sync";
                    last_sync_revision.reset();
                }
            }
        }

        else
        {
            SYNCLOG_INFO << "First time GET with this sync service";
        }

        int start_serial_number = -1;
        std::string server_revision;
        std::string last_case_uuid;

        if( last_sync_revision.has_value() )
        {
            start_serial_number = last_sync_revision->GetSerialNumber();
            server_revision = last_sync_revision->GetServerFileRevision();

            if( last_sync_revision->IsPartialGet() )
                last_case_uuid = last_sync_revision->GetLastCaseUuid();
        }

        std::vector<std::string> excluded_revisions = GetPutRevisionsSince(GetServerDeviceId(), repository, start_serial_number);

        IDataChunk& data_chunk = m_syncService->GetChunk();
        data_chunk.EnableOptimization();

        repository.StartSync(GetServerDeviceId(), m_connectResponse->GetServerName(), m_connectResponse->GetUsername(),
                             SyncDirection::Get, universe, UseRemoteCaseOnConflict);
        DataRepositoryTransaction transaction(repository);

        std::optional<int> total_cases;
        int cases_so_far = 0;

        while( true )
        {
            // we will update progress below based on cases so disable default handling
            if( m_syncListener != nullptr && cases_so_far != 0 )
                m_syncListener->ShowProgressUpdates(false);

            // send request to sync service
            SyncGetResponse response = m_syncService->GetCases(repository.GetSharedCaseAccess(),
                                                               m_deviceId, universe, server_revision,
                                                               last_case_uuid, excluded_revisions);

            if( response.GetResult() == SyncGetResponse::SyncGetResult::RevisionNotFound )
            {
                SYNCLOG_INFO << "Previous revision not found, doing full sync";

                // Server revision history is out of sync with local revision history. Fallback to doing a full sync instead
                // of changes since last sync.
                last_sync_revision.reset();
                server_revision.clear();
                last_case_uuid.clear();
                excluded_revisions.clear();
            }

            else
            {
                // OK or more data
                server_revision = response.GetServerRevision();

                if( !total_cases.has_value() )
                {
                    total_cases = response.GetTotalCases();

                    if( m_syncListener != nullptr && total_cases.has_value() )
                        m_syncListener->SetProgressTotal(*total_cases);
                }

                std::string last_processed_case_uuid;

                if( response.GetCases() != nullptr )
                {
                    std::exception_ptr caught_exception;

                    response.GetCases()->subscribe(
                        [this, &repository, server_revision, &last_processed_case_uuid, &cases_so_far](std::shared_ptr<Case> data_case)
                        {
                            last_processed_case_uuid = data_case->GetUuid();

                            const std::vector<std::shared_ptr<Case>> cases_received { std::move(data_case) };
                            repository.SyncCasesFromRemote(cases_received, server_revision);

                            if( m_syncListener != nullptr )
                            {
                                m_syncListener->ShowProgressUpdates(true);
                                m_syncListener->Progress(++cases_so_far);
                                m_syncListener->ShowProgressUpdates(false);

                                m_syncListener->SetLastCaseSynced(*cases_received.back(), true);
                            }
                        },
                        [&](std::exception_ptr exception)
                        {
                            caught_exception = exception;
                        }
                    );

                    if( caught_exception )
                        RethrowAsSyncError(caught_exception);
                }

                if( response.GetResult() == SyncGetResponse::SyncGetResult::Complete )
                    break;

                // Start next chunk after last UUID received
                last_case_uuid = std::move(last_processed_case_uuid);
            }
        }

        repository.EndSync();

        data_chunk.ResetOptimization();

        const ISyncableDataRepository::SyncStats stats = repository.GetLastSyncStats();

        SYNCLOG_INFO << "New sync service revision = " << server_revision;
        SYNCLOG_INFO << "Sync GET completed. ";
        SYNCLOG_INFO << "Downloaded " << stats.cases_received << " cases";
        SYNCLOG_INFO << stats.cases_not_in_repository << " new cases, "
                     << stats.cases_newer_on_remote << " updated, "
                     << stats.cases_newer_in_repository << " ignored, "
                     << stats.cases_with_conflicts << " conflicts";
    }

    catch( const DataRepositoryException::Error& exception )
    {
        SYNCLOG_ERROR << "Database error during sync GET: " << exception.what();
        throw SyncError(100133, exception);
    }
}


void SyncClient::SyncDataPut(ISyncableDataRepository& repository, const std::string& universe)
{
    try
    {
        DeviceId exclude_gets_from_device_id = GetServerDeviceId();
        std::optional<SyncHistoryEntry> last_sync_revision = GetRevisionFromLastSync(SyncDirection::Put, repository, universe);

        if( last_sync_revision.has_value() )
        {
            SYNCLOG_INFO << "Last PUT with this sync service " << DateTime::LocalDateTimeString(last_sync_revision->GetDateTime())
                         << " local revision " << last_sync_revision->GetFileRevision()
                         << " sync service revision \"" << last_sync_revision->GetServerFileRevision() << "\"";

            if( last_sync_revision->IsPartialPut() )
            {
                SYNCLOG_INFO << "Partial put, resume from case " << last_sync_revision->GetLastCaseUuid();

                // Invalid to try to resume a sync with different params
                if( last_sync_revision->GetUniverse() != universe )
                {
                    SYNCLOG_INFO << "Universe doesn't match universe from partial - doing full sync";

                    repository.ClearBinarySyncHistory(exclude_gets_from_device_id);

                    exclude_gets_from_device_id.clear();
                    last_sync_revision.reset();
                }
            }
        }

        else
        {
            SYNCLOG_INFO << "First time PUT with this sync service";
        }


        std::string server_revision;
        int client_revision = -1;
        std::string last_case_uuid;

        if( last_sync_revision.has_value() )
        {
            client_revision = last_sync_revision->GetFileRevision();

            // When uploading multiple chunks we store revisions as a comma separated list
            // so we need to grab the last one to get the most recent sync put revision
            if( !last_sync_revision->GetServerFileRevision().empty() )
                server_revision = SO::SplitString(last_sync_revision->GetServerFileRevision(), ',').back();

            if( last_sync_revision->IsPartialPut() )
                last_case_uuid = last_sync_revision->GetLastCaseUuid();
        }

        IDataChunk& data_chunk = m_syncService->GetChunk();
        data_chunk.EnableOptimization();

        size_t case_count = 0;
        int max_client_revision = 0;
        std::unique_ptr<CaseIterator> case_iterator = repository.GetCasesModifiedSinceRevisionIterator(client_revision, last_case_uuid, universe, data_chunk.GetCaseSize(),
                                                                                                       &case_count, &max_client_revision, exclude_gets_from_device_id);

        SYNCLOG_INFO << "Total new/modified cases since last sync: " << case_count;

        if( m_syncListener != nullptr )
            m_syncListener->SetProgressTotal(case_count);

        repository.StartSync(GetServerDeviceId(), m_connectResponse->GetServerName(), m_connectResponse->GetUsername(),
                             SyncDirection::Put, universe, UseRemoteCaseOnConflict);

        const std::unique_ptr<SyncBinaryDataUploadManager> sync_binary_data_upload_manager =
            ( repository.GetCaseAccess().GetCaseMetadata().UsesBinaryData() ) ? std::make_unique<SyncableDataRepositorySyncBinaryDataUploadManager>(repository, GetServerDeviceId()) :
                                                                                nullptr;

        std::vector<std::shared_ptr<Case>> cases_pool;
        size_t cases_sent = 0;
        std::string all_returned_revisions;

        while( true )
        {
            // read the cases in this chunk, keeping track of the binary data size
            size_t num_cases_in_chunk = 0;

            if( sync_binary_data_upload_manager != nullptr )
                sync_binary_data_upload_manager->ResetForNextChunk();

            while( true )
            {
                Case& this_case = ( num_cases_in_chunk < cases_pool.size() ) ? *cases_pool[num_cases_in_chunk] :
                                                                               *cases_pool.emplace_back(repository.GetCaseAccess().CreateCase());

                if( !case_iterator->NextCase(this_case) )
                    break;

                ++num_cases_in_chunk;

                if( sync_binary_data_upload_manager != nullptr )
                {
                    sync_binary_data_upload_manager->AnalyzeCaseBinaryData(this_case);

                    if( sync_binary_data_upload_manager->GetBinaryDataSizeOfChunk() > data_chunk.GetBinaryContentSize() )
                    {
                        SYNCLOG_INFO << "Binary items in the chunk exceeded (" << sync_binary_data_upload_manager->GetBinaryDataSizeOfChunk()
                                     << ") << the set limit of chunk size (" << data_chunk.GetBinaryContentSize()
                                     << "). Sending a subchunk of cases: " << num_cases_in_chunk << " cases";
                        break;
                    }
                }
            }

            // we will update progress based on cases, so disable default progress
            if( m_syncListener != nullptr )
                m_syncListener->ShowProgressUpdates(false);

            const std::vector<Case*> cases_in_chunk = SpanHelpers::CreatePointersSpan(cases_pool, num_cases_in_chunk);

            const SyncPutResponse response = m_syncService->PutCases(repository.GetSharedCaseAccess(),
                                                                     cases_in_chunk, sync_binary_data_upload_manager.get(),
                                                                     m_deviceId, universe, server_revision);

            if( response.GetResult() == SyncPutResponse::SyncPutResult::RevisionNotFound )
            {
                SYNCLOG_INFO << "Previous revision not found, doing full sync";

                // Server revision history is out of sync with local revision history. Fallback to doing a full sync instead
                // of changes since last sync.
                client_revision = 0;
                server_revision.clear();
                exclude_gets_from_device_id.clear();
                last_case_uuid.clear();

                // clear the binary sync to this device
                repository.ClearBinarySyncHistory(GetServerDeviceId());

                // Get next chunk of cases
                case_iterator = repository.GetCasesModifiedSinceRevisionIterator(client_revision, last_case_uuid, universe, data_chunk.GetCaseSize(),
                                                                                 &case_count, &max_client_revision, exclude_gets_from_device_id);

                if( m_syncListener != nullptr )
                    m_syncListener->SetProgressTotal(case_count);
            }

            else
            {
                cases_sent += num_cases_in_chunk;
                SYNCLOG_INFO << "Uploaded chunk of " << num_cases_in_chunk << " cases";

                if( m_syncListener != nullptr )
                {
                    m_syncListener->ShowProgressUpdates(true);
                    m_syncListener->Progress(cases_sent);

                    if( num_cases_in_chunk != 0 )
                    {
                        ASSERT(num_cases_in_chunk <= cases_pool.size());
                        m_syncListener->SetLastCaseSynced(*cases_pool[num_cases_in_chunk - 1], false);
                    }
                }

                // Since a single sync on client can correspond to multiple server revisions we store
                // a comma separated list of server revisions in database
                server_revision = response.GetServerRevision();
                SO::AppendWithSeparator(all_returned_revisions, server_revision, ",");

                repository.MarkCasesSentToRemote(cases_in_chunk, sync_binary_data_upload_manager.get(), all_returned_revisions, max_client_revision);

                // break when all cases have been sent
                if( cases_sent >= case_count )
                    break;

                last_case_uuid = cases_in_chunk.back()->GetUuid();
                client_revision = max_client_revision;

                // Get next chunk of cases
                case_iterator = repository.GetCasesModifiedSinceRevisionIterator(client_revision, last_case_uuid, universe, data_chunk.GetCaseSize(),
                                                                                 nullptr, &max_client_revision, exclude_gets_from_device_id);
            }
        }

        repository.EndSync();

        data_chunk.ResetOptimization();

        const ISyncableDataRepository::SyncStats stats = repository.GetLastSyncStats();

        SYNCLOG_INFO << "New sync service revision = " << server_revision;
        SYNCLOG_INFO << "Sync PUT completed. ";
        SYNCLOG_INFO << "Uploaded " << stats.cases_sent << " cases";
    }

    catch( const DataRepositoryException::Error& exception )
    {
        SYNCLOG_ERROR << "Database error during sync PUT: " << exception.what();
        throw SyncError(100133, exception);
    }
}


std::optional<SyncHistoryEntry> SyncClient::GetRevisionFromLastSync(const SyncDirection direction, ISyncableDataRepository& repository, const std::string& universe) const
{
    std::optional<SyncHistoryEntry> last_sync_revision = repository.GetLastSyncForDevice(GetServerDeviceId(), direction);

    // only use the previous revision number if the universe stayed the same or became more restrictive
    if( last_sync_revision.has_value() && !SO::StartsWith(universe, last_sync_revision->GetUniverse()) )
        last_sync_revision.reset();

    return last_sync_revision;
}


std::vector<std::string> SyncClient::GetPutRevisionsSince(const DeviceId& device_id, ISyncableDataRepository& repository, const int start_serial_number)
{
    std::vector<std::string> revisions;

    for( const SyncHistoryEntry& sync_history_entry : repository.GetSyncHistory(device_id, SyncDirection::Put, start_serial_number) )
    {
        if( !sync_history_entry.GetServerFileRevision().empty() )
            revisions.emplace_back(sync_history_entry.GetServerFileRevision());
    }

    return revisions;
}

#endif // REFERENCE
