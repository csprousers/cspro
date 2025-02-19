#pragma once

#include <zDataO/zDataO.h>
#include <zToolsO/span.h>
#include <zCaseO/CaseItemIndex.h>

class BinaryCaseItem;
class BinaryDataMetadata;
class CaseAccess;
class ISyncableDataRepository;


// --------------------------------------------------------------------------
// SyncBinaryDataUploadManager
// --------------------------------------------------------------------------

class ZDATAO_API SyncBinaryDataUploadManager
{
public:
    SyncBinaryDataUploadManager();
    SyncBinaryDataUploadManager(const SyncBinaryDataUploadManager&) = delete;
    SyncBinaryDataUploadManager(SyncBinaryDataUploadManager&&) = delete;

    virtual ~SyncBinaryDataUploadManager() { }

    // Clears all information about binary data that needs to be synced as part of this chunk.
    void ResetForNextChunk();

    // Determines what binary data from the case, or group of cases, needs to be included in this chunk.
    void AnalyzeCaseBinaryData(const Case& data_case);
    void AnalyzeCaseBinaryData(const cs::span<const Case* const> cases);

    // Returns true if binary data is part of this chunk.
    bool IsBinaryDataPartOfChunk() const { return !m_binaryCaseItemsToSync.empty(); }

    // Returns the total number of bytes in the binary data in the chunk.
    uint64_t GetBinaryDataSizeOfChunk() const { return m_totalBinaryDataBytes; }

    // Executes the callback function for all binary data that is part of the chunk.
    // The second version executes the callback function with the binary case item's signature.
    void ForeachBinaryCaseItemInChunk(const std::function<void(const BinaryCaseItem&, const CaseItemIndex&)>& callback_function) const;
    void ForeachBinaryCaseItemInChunk(const std::function<void(const std::string&)>& callback_function) const;

protected:
    virtual void AddBinarySignaturesNotSyncedWithRemote(const Case& data_case, std::vector<std::string>& signatures_to_sync) = 0;

private:
    // all signatures ever synced (or intended for syncing in the next chunk)
    std::vector<std::string> m_signaturesSyncedOrToSync;

    // binary data that is part of this sync's chunk
    uint64_t m_totalBinaryDataBytes;
    std::vector<std::tuple<const BinaryCaseItem&, const CaseItemIndex>> m_binaryCaseItemsToSync;
};



// --------------------------------------------------------------------------
// SyncableDataRepositorySyncBinaryDataUploadManager
// --------------------------------------------------------------------------

class ZDATAO_API SyncableDataRepositorySyncBinaryDataUploadManager : public SyncBinaryDataUploadManager
{
public:
    SyncableDataRepositorySyncBinaryDataUploadManager(ISyncableDataRepository& syncable_data_repository, DeviceId server_device_id);

protected:
    void AddBinarySignaturesNotSyncedWithRemote(const Case& data_case, std::vector<std::string>& signatures_to_sync) override;

private:
    ISyncableDataRepository& m_syncableDataRepository;
    DeviceId m_serverDeviceId;
};
