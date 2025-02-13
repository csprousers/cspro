#pragma once

#include <zUtilO/BinaryDataReader.h>


class SyncJsonBinaryDataReader : public BinaryDataReader
{
public:
    SyncJsonBinaryDataReader(BinaryDataMetadata metadata)
        :   m_metadata(std::move(metadata))
    {
        ASSERT(!m_metadata.GetBinaryDataKey().empty());
    }

    const BinaryDataMetadata& GetMetadata() const
    {
        return m_metadata;
    }

    bool HasSyncedContent() const
    {
        return ( m_syncedContent != nullptr );
    }

    std::shared_ptr<const std::vector<std::byte>> GetSyncedContent() const
    {
        return m_syncedContent;
    }

    void SetSyncedContent(std::shared_ptr<const std::vector<std::byte>> content)
    {
        ASSERT(m_syncedContent == nullptr && content != nullptr);
        m_syncedContent = std::move(content);
    }

    // BinaryDataReader overrides
    // --------------------------------------------------------------------------
    BinaryData GetBinaryData() override
    {
        ASSERT(m_syncedContent != nullptr);
        return BinaryData(m_syncedContent, m_metadata);
    }

    const BinaryDataMetadata& GetMetadata() override
    {
        return m_metadata;
    }    

    uint64_t GetSize() override
    {
        // the size should not be queried as part of a sync
        return ReturnProgrammingError(0);
    }

    void OnBinaryDataChange() override
    {
        // the data should not changed as part of a sync
        ASSERT(false);
    }

private:
    const BinaryDataMetadata m_metadata;
    std::shared_ptr<const std::vector<std::byte>> m_syncedContent;
};
