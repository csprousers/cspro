#pragma once

#include <zUtilO/BinaryDataReader.h>
#include <zDataO/SQLiteBinaryItemSerializer.h>


class SQLiteBinaryDataReader : public BinaryDataReader
{
public:
    SQLiteBinaryDataReader(const SQLiteBinaryItemSerializer* serializer, BinaryDataMetadata metadata)
        :   m_serializer(serializer),
            m_metadata(std::move(metadata)),
            m_dataChanged(false)
    {
        ASSERT(m_serializer != nullptr && !m_metadata.GetBinaryDataKey().empty());
    }

    const BinaryDataMetadata& GetMetadata() const
    {
        return m_metadata;
    }

    bool IsUpToDate(const SQLiteBinaryItemSerializer* serializer) const
    {
        // if the data came from this repository and wasn't changed, we don't need to write it out again
        return ( !m_dataChanged && serializer == m_serializer );
    }

    // BinaryDataReader overrides
    // --------------------------------------------------------------------------
    BinaryData GetBinaryData() override
    {
        return m_serializer->GetBinaryItemData(m_metadata);
    }

    const BinaryDataMetadata& GetMetadata() override
    {
        return m_metadata;
    }

    uint64_t GetSize() override
    {
        return m_serializer->GetBinaryItemSize(m_metadata);
    }

    void OnBinaryDataChange() override
    {
        m_dataChanged = true;
    }

private:
    const SQLiteBinaryItemSerializer* m_serializer;
    const BinaryDataMetadata m_metadata;
    bool m_dataChanged;
};
