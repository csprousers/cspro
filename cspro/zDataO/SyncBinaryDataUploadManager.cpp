#include "stdafx.h"
#include "SyncBinaryDataUploadManager.h"
#include "ISyncableDataRepository.h"
#include <zToolsO/VectorHelpers.h>


// --------------------------------------------------------------------------
// SyncBinaryDataUploadManager
// --------------------------------------------------------------------------

SyncBinaryDataUploadManager::SyncBinaryDataUploadManager()
    :   m_totalBinaryDataBytes(0)
{
}


void SyncBinaryDataUploadManager::ResetForNextChunk()
{
    m_totalBinaryDataBytes = 0;
    m_binaryCaseItemsToSync.clear();
}


void SyncBinaryDataUploadManager::AnalyzeCaseBinaryData(const Case& data_case)
{
    size_t num_signatures_accounted_for = m_signaturesSyncedOrToSync.size();

    AddBinarySignaturesNotSyncedWithRemote(data_case, m_signaturesSyncedOrToSync);

    // return if there is no binary data to be synced in this case
    if( ( num_signatures_accounted_for == m_signaturesSyncedOrToSync.size() ) ||
        ( VectorHelpers::RemoveDuplicates(m_signaturesSyncedOrToSync) > 0 && num_signatures_accounted_for == m_signaturesSyncedOrToSync.size() ) )
    {
        return;
    }

    // create references to the binary data that needs to be synced
    data_case.ForeachDefinedBinaryCaseItem(
        [&](const BinaryCaseItem& binary_case_item, const CaseItemIndex& index)
        {
            try
            {
                const BinaryDataAccessor& binary_data_accessor = binary_case_item.GetBinaryDataAccessor(index);
                const std::string& signature = binary_data_accessor.GetSignature();

                // only include data that has not already been processed
                // (either in a prior sync, or already in this method in the off chance that a case has duplicate binary data)
                auto first_unaccounted_for_entry = m_signaturesSyncedOrToSync.begin() + num_signatures_accounted_for;
                auto lookup = std::find(first_unaccounted_for_entry,
                                        m_signaturesSyncedOrToSync.end(),
                                        signature);

                if( lookup != m_signaturesSyncedOrToSync.end() )
                {
                    m_totalBinaryDataBytes += binary_data_accessor.GetBinaryDataSize();
                    m_binaryCaseItemsToSync.emplace_back(binary_case_item, index);

                    // swap this, now accounted for entry, with the first unaccounted for entry
                    if( lookup != first_unaccounted_for_entry )
                        std::swap(*lookup, *first_unaccounted_for_entry);

                    ++num_signatures_accounted_for;
                }
            }
            catch(...) { ASSERT(false); }
        });

    ASSERT(num_signatures_accounted_for == m_signaturesSyncedOrToSync.size());
}


void SyncBinaryDataUploadManager::AnalyzeCaseBinaryData(const cs::span<const Case* const> cases)
{
    for( const Case* const data_case : cases )
        AnalyzeCaseBinaryData(*data_case);
}


void SyncBinaryDataUploadManager::ForeachBinaryCaseItemInChunk(const std::function<void(const BinaryCaseItem&, const CaseItemIndex&)>& callback_function) const
{
    for( const auto& [binary_case_item, index] : m_binaryCaseItemsToSync )
        callback_function(binary_case_item, index);
}


void SyncBinaryDataUploadManager::ForeachBinaryCaseItemInChunk(const std::function<void(const std::string&)>& callback_function) const
{
    for( const auto& [binary_case_item, index] : m_binaryCaseItemsToSync )
        callback_function(binary_case_item.GetBinaryDataAccessor(index).GetSignature());
}



// --------------------------------------------------------------------------
// SyncableDataRepositorySyncBinaryDataUploadManager
// --------------------------------------------------------------------------

SyncableDataRepositorySyncBinaryDataUploadManager::SyncableDataRepositorySyncBinaryDataUploadManager(ISyncableDataRepository& syncable_data_repository, DeviceId server_device_id)
    :   m_syncableDataRepository(syncable_data_repository),
        m_serverDeviceId(std::move(server_device_id))
{
    ASSERT(m_syncableDataRepository.GetCaseAccess().GetCaseMetadata().UsesBinaryData());
}


void SyncableDataRepositorySyncBinaryDataUploadManager::AddBinarySignaturesNotSyncedWithRemote(const Case& data_case, std::vector<std::string>& signatures_to_sync)
{
    m_syncableDataRepository.AddBinarySignaturesNotSyncedWithRemote(data_case, m_serverDeviceId, signatures_to_sync);
}
