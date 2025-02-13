#pragma once

#include <zUtilO/BinaryDataMetadata.h>


class BinaryData
{
public:
    // the content for BinaryData will typically be provided as a vector of bytes;
    // the content can also be created on demand by using a callback function;
    // the callback function must not throw exceptions
    using ContentCallbackType = std::function<std::shared_ptr<const std::vector<std::byte>>()>;

    BinaryData(std::shared_ptr<const std::vector<std::byte>> content, BinaryDataMetadata metadata);
    BinaryData(std::vector<std::byte> content, BinaryDataMetadata metadata);

    BinaryData(std::shared_ptr<ContentCallbackType> content_callback, BinaryDataMetadata metadata);
    BinaryData(ContentCallbackType content_callback, BinaryDataMetadata metadata);

    const std::vector<std::byte>& GetContent() const;
    std::shared_ptr<const std::vector<std::byte>> GetSharedContent() const;

    const BinaryDataMetadata& GetMetadata() const { return m_metadata; }
    BinaryDataMetadata& GetMetadata()             { return m_metadata; }

private:
    void GetContentUsingCallback() const;

private:
    using ContentStorageType = std::shared_ptr<const std::vector<std::byte>>;
    using ContentCallbackStorageType = std::shared_ptr<ContentCallbackType>;

    std::variant<ContentStorageType, ContentCallbackStorageType> m_contentOrCallback;
    BinaryDataMetadata m_metadata;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline BinaryData::BinaryData(std::shared_ptr<const std::vector<std::byte>> content, BinaryDataMetadata metadata)
    :   m_contentOrCallback(std::move(content)),
        m_metadata(std::move(metadata))
{
    ASSERT(std::get<ContentStorageType>(m_contentOrCallback) != nullptr);
}


inline BinaryData::BinaryData(std::vector<std::byte> content, BinaryDataMetadata metadata)
    :   BinaryData(std::make_unique<const std::vector<std::byte>>(std::move(content)), std::move(metadata))
{
}


inline BinaryData::BinaryData(std::shared_ptr<ContentCallbackType> content_callback, BinaryDataMetadata metadata)
    :   m_contentOrCallback(std::move(content_callback)),
        m_metadata(std::move(metadata))
{
    ASSERT(std::get<ContentCallbackStorageType>(m_contentOrCallback) != nullptr &&
           std::get<ContentCallbackStorageType>(m_contentOrCallback)->operator bool());
}


inline BinaryData::BinaryData(ContentCallbackType content_callback, BinaryDataMetadata metadata)
    :   BinaryData(std::make_unique<ContentCallbackType>(std::move(content_callback)), std::move(metadata))
{
}


inline const std::vector<std::byte>& BinaryData::GetContent() const
{
    if( std::holds_alternative<ContentCallbackStorageType>(m_contentOrCallback) )
        GetContentUsingCallback();    

    return *std::get<ContentStorageType>(m_contentOrCallback);
}


inline std::shared_ptr<const std::vector<std::byte>> BinaryData::GetSharedContent() const
{
    if( std::holds_alternative<ContentCallbackStorageType>(m_contentOrCallback) )
        GetContentUsingCallback();    

    return std::get<ContentStorageType>(m_contentOrCallback);
}


inline void BinaryData::GetContentUsingCallback() const
{
    ASSERT(std::holds_alternative<ContentCallbackStorageType>(m_contentOrCallback));

#ifdef _DEBUG
    try
#endif
    {
        const_cast<BinaryData*>(this)->m_contentOrCallback = (*std::get<ContentCallbackStorageType>(m_contentOrCallback))();
    }
#ifdef _DEBUG
    catch(...)
    {
        ASSERT(false);
        throw;
    }
#endif

    ASSERT(std::holds_alternative<ContentStorageType>(m_contentOrCallback));
}
