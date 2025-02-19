#pragma once

#include <zUtilO/BinaryContentReader.h>


// --------------------------------------------------------------------------
// SyncBinaryContentReader
//
// This binary content reader is used to store information about binary items
// read during a sync. How the binary data itself is processed is handled by
// subclasses.
// --------------------------------------------------------------------------

class SyncBinaryContentReader : public BinaryContentReader
{
public:
    SyncBinaryContentReader(std::optional<uint64_t> size);

    // BinaryContentReader overrides
    uint64_t GetSize(const std::string& signature) override;

protected:
    std::optional<uint64_t> m_size;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline SyncBinaryContentReader::SyncBinaryContentReader(std::optional<uint64_t> size)
    :   m_size(std::move(size))
{
}


inline uint64_t SyncBinaryContentReader::GetSize(const std::string& signature)
{
    // the size should be set in the JSON, but if not, get the data to calculate the size
    if( !m_size.has_value() )
    {
        ASSERT(false);
        m_size = GetContent(signature)->size();
    }

    return *m_size;
}
