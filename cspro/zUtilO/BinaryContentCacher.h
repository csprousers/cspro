#pragma once

#include <zUtilO/zUtilO.h>


class CLASS_DECL_ZUTILO BinaryContentCacher
{
    static constexpr size_t MaxBytesToHoldInSingleUseCountStorage = 100 * 1024 * 1024; // 100 MB

public:
    class CacheableContent;

    // Retrieves the cached binary content identified by the signature.
    // Null is returned if no such data is cached.
    static std::shared_ptr<const std::vector<std::byte>> Retrieve(const std::string& signature) { return Retrieve(GetData(), signature); }

    // Caches the binary content, with the cached content returned.
    // If binary content is already cached using the signature, the existing content is used
    // instead of having different shared pointers pointing to what would be the same content.
    // If the total bytes cached, held by shared pointers with only one use count, exceeds
    // MaxBytesToHoldInSingleUseCountStorage, some cached data will be removed.
    static std::shared_ptr<const std::vector<std::byte>> Store(const std::string& signature, CacheableContent cacheable_content);

    // Clears the cache.
    static void ClearCache();

private:
    struct Data;
    static Data& GetData();

    static std::shared_ptr<const std::vector<std::byte>> Retrieve(const Data& data, const std::string& signature);

    static void PruneCache(Data& data);
};



class CLASS_DECL_ZUTILO BinaryContentCacher::CacheableContent
{
    friend class BinaryContentCacher;

public:
    // Creates an object of cacheable content, which is simply an object with a shared_ptr whose
    // destructor is part of zUtilO, which alleviates concerns about shared pointers destructors'
    // being part of a DLL that will no longer be accessible when clearing the cache.
    CacheableContent(std::vector<std::byte> content);

private:
    std::shared_ptr<const std::vector<std::byte>> m_content;
};
