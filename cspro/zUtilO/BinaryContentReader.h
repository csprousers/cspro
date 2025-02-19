#pragma once

#include <zUtilO/BinaryContentCacher.h>
#include <zToolsO/UniqueId.h>


// --------------------------------------------------------------------------
// BinaryContentReader
//
// Any object, generally a data repository, can create a subclass of the
// BinaryContentReader, which will be called by BinaryDataAccessor, generally
// as part of a BinaryCaseItem.
//
// Data repositories can assume that the BinaryContentReader will only be
// used while the data repository is still open, as closing a data repository
// should trigger a call to Case::LoadAllBinaryData.
//
// GetSize and GetContent can throw exceptions.
//
// Content read will be cached using the BinaryContentCacher.
// --------------------------------------------------------------------------

class BinaryContentReader
{
public:
    virtual ~BinaryContentReader() { }

    // Returns an ID that identifies the creator of this object (or null if not applicable).
    virtual const UniqueId* GetUniqueId() const = 0;

    // Returns the size of the binary content (potentially without having to load the content).
    virtual uint64_t GetSize(const std::string& signature) = 0;

    // Returns the binary content.
    std::shared_ptr<const std::vector<std::byte>> GetContent(const std::string& signature);

private:
    virtual BinaryContentCacher::CacheableContent GetContentWorker(const std::string& signature) = 0;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline std::shared_ptr<const std::vector<std::byte>> BinaryContentReader::GetContent(const std::string& signature)
{
    std::shared_ptr<const std::vector<std::byte>> content = BinaryContentCacher::Retrieve(signature);

    if( content == nullptr )
        content = BinaryContentCacher::Store(signature, GetContentWorker(signature));

    return content;
}
