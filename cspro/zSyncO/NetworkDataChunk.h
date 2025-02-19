#pragma once

#include <zSyncO/zSyncO.h>
#include <zSyncO/IDataChunk.h>
#include <chrono>


// --------------------------------------------------------------------------
// NetworkDataChunk
//
// A data chunk implementation for network-based connections such as CSWeb,
// Dropbox, etc.).
// --------------------------------------------------------------------------

class SYNC_API NetworkDataChunk : public IDataChunk
{
public:
    NetworkDataChunk(size_t case_size);
    NetworkDataChunk();

    size_t GetCaseSize() const override;
    uint64_t GetBinaryContentSize() const override;

    void EnableOptimization() override { /* do nothing */ }
    void ResetOptimization() override  { /* do nothing */ }

    void ResetForNextChunk();

    virtual void Optimize(size_t cases_in_last_chunk, size_t case_json_bytes_in_last_chunk);

    void OnError();

private:
    size_t m_caseSize;
    size_t m_lastGoodCaseSize;
    std::optional<std::chrono::time_point<std::chrono::steady_clock>> m_chunkStartTime;
};



// --------------------------------------------------------------------------
// NetworkDataChunkWithoutOptimizations
//
// An implementation for file-based connections that use the local file
// system, such as Dropbox (Local), that does not optimize chunks because the
// speed at which cases are uploaded/downloaded will result in huge chunks
// that would then be poorly handled by the connection when accessed via a
// network.
// --------------------------------------------------------------------------

class NetworkDataChunkWithoutOptimizations : public NetworkDataChunk
{
public:
    using NetworkDataChunk::NetworkDataChunk;

    void Optimize(size_t /*cases_in_last_chunk*/, size_t /*case_json_bytes_in_last_chunk*/) override { }
};
