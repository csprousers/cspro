#pragma once

#include <zUtilO/BinaryContentReader.h>
#include <zDataO/SQLiteBinaryItemSerializer.h>


class SQLiteBinaryContentReader : public BinaryContentReader
{
public:
    SQLiteBinaryContentReader(UniqueId repository_id, SQLiteBinaryItemSerializer* const binary_item_serializer)
        :   m_repositoryId(std::move(repository_id)),
            m_binaryItemSerializer(binary_item_serializer)
            
    {
        ASSERT(m_binaryItemSerializer != nullptr);
    }
    
    // BinaryContentReader overrides
    const UniqueId* GetUniqueId() const override
    {
        return &m_repositoryId;
    }

    uint64_t GetSize(const std::string& signature) override
    {
        return m_binaryItemSerializer->GetContentSize(signature);
    }

protected:
    BinaryContentCacher::CacheableContent GetContentWorker(const std::string& signature) override
    {
        return m_binaryItemSerializer->GetContent(signature);
    }

private:
    UniqueId m_repositoryId;
    SQLiteBinaryItemSerializer* m_binaryItemSerializer;
};
