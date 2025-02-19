#include "StdAfx.h"
#include "BinaryContentCacher.h"


struct BinaryContentCacher::Data
{
    size_t bytes_cached = 0;
    std::vector<std::tuple<std::string, std::shared_ptr<const std::vector<std::byte>>>> cached_content;
};


BinaryContentCacher::Data& BinaryContentCacher::GetData()
{
    static Data data;
    return data;
}


std::shared_ptr<const std::vector<std::byte>> BinaryContentCacher::Retrieve(const Data& data, const std::string& signature)
{
    ASSERT(!signature.empty());

    const auto& lookup = std::find_if(data.cached_content.cbegin(), data.cached_content.cend(),
                                      [&](const auto& signature_and_content) { return ( signature == std::get<0>(signature_and_content) ); });

    if( lookup != data.cached_content.cend() )
        return std::get<1>(*lookup);

    return nullptr;
}


std::shared_ptr<const std::vector<std::byte>> BinaryContentCacher::Store(const std::string& signature, CacheableContent cacheable_content)
{
    std::shared_ptr<const std::vector<std::byte>> content = std::move(cacheable_content.m_content);
    ASSERT(!signature.empty() && content != nullptr);

    Data& data = GetData();
    std::shared_ptr<const std::vector<std::byte>> already_cached_content = Retrieve(data, signature);

    // if the content was already cached, we'll reuse it
    if( already_cached_content != nullptr )
    {
        ASSERT(already_cached_content != content);
        ASSERT(already_cached_content->size() == content->size());
        ASSERT(memcmp(already_cached_content->data(), content->data(), content->size()) == 0);

        return already_cached_content;
    }

    // insert the new content
    data.cached_content.emplace_back(signature, content);
    data.bytes_cached += content->size();

    // prune the cache if necessary
    if( data.bytes_cached > MaxBytesToHoldInSingleUseCountStorage )
    {
        ASSERT(content.use_count() > 1);
        PruneCache(data);
    }

    return content;
}


void BinaryContentCacher::ClearCache()
{
    Data& data = GetData();

    data.bytes_cached = 0;
    data.cached_content.clear();
}


void BinaryContentCacher::PruneCache(Data& data)
{
    ASSERT(data.bytes_cached > MaxBytesToHoldInSingleUseCountStorage);

    // remove content in order of when it was cached (oldest cached first)
    auto cache_itr = data.cached_content.begin();

    while( cache_itr != data.cached_content.end() )
    {
        if( std::get<1>(*cache_itr).use_count() > 1 )
        {
            ++cache_itr;
        }

        else
        {
            data.bytes_cached -= std::get<1>(*cache_itr)->size();
            cache_itr = data.cached_content.erase(cache_itr);

            if( data.bytes_cached <= MaxBytesToHoldInSingleUseCountStorage )
                return;
        }
    }
}


BinaryContentCacher::CacheableContent::CacheableContent(std::vector<std::byte> content)
    :   m_content(std::make_shared<const std::vector<std::byte>>(std::move(content)))
{
}
