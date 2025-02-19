#pragma once

#include <zDataO/SyncBinaryContentReader.h>
#include <zCaseO/CaseJsonSerializer.h>


// --------------------------------------------------------------------------
// SyncWithDataBinaryContentReader
//
// This binary content reader is used during a sync operation where the
// binary data is included after the case JSON.
// --------------------------------------------------------------------------

class SyncWithDataBinaryContentReader : public SyncBinaryContentReader
{
public:
    using SyncBinaryContentReader::SyncBinaryContentReader;

    bool ContentReceivedDuringSync() const
    {
        return m_content.has_value();
    }

    void SetContent(BinaryContentCacher::CacheableContent content)
    {
        ASSERT(!m_content.has_value());
        m_content = std::move(content);
    }

    // BinaryContentReader overrides
    const UniqueId* GetUniqueId() const override
    {
        return nullptr;
    }

protected:
    BinaryContentCacher::CacheableContent GetContentWorker(const std::string& /*signature*/) override
    {
        if( !m_content.has_value() )
            return ReturnProgrammingError(std::vector<std::byte>());

        return *m_content;
    }

private:
    std::optional<BinaryContentCacher::CacheableContent> m_content;
};
