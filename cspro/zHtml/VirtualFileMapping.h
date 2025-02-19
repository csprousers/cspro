#pragma once

#include <zHtml/zHtml.h>


// --------------------------------------------------------------------------
// VirtualFileMapping
// 
// an object that controls the lifecycle of a single virtual file mapping
// --------------------------------------------------------------------------

class VirtualFileMapping
{
    friend class LocalFileServer;
    friend class PortableLocalFileServer;
    friend class SharedHtmlLocalFileServer;

private:
    VirtualFileMapping(std::string url, std::shared_ptr<bool> mapping_active)
        :   m_url(std::move(url)),
            m_mappingActive(std::move(mapping_active))
    {
        ASSERT(m_mappingActive != nullptr && *m_mappingActive);
    }

public:
    VirtualFileMapping(const VirtualFileMapping&) = delete;
    VirtualFileMapping(VirtualFileMapping&& rhs) = default;

    ~VirtualFileMapping()
    {
        if( m_mappingActive != nullptr )
            *m_mappingActive = false;
    }

    const std::string& GetUrl() { return m_url; }

private:
    std::string m_url;
    std::shared_ptr<bool> m_mappingActive;
};



// --------------------------------------------------------------------------
// VirtualFileMappingResponse
// 
// an object that the virtual file mappers can use to return data without
// needing to know details about the local file server
// --------------------------------------------------------------------------

class VirtualFileMappingResponse
{
public:
    VirtualFileMappingResponse(void* response_object);

    ZHTML_API void SetContent(const void* content_data, size_t content_size, cs::string_sz content_type);

    void SetContent(std::string_view content_sv, cs::string_sz content_type);

    void SetContent(const std::vector<std::byte>& content, cs::string_sz content_type);

    void SetContent(std::shared_ptr<const std::vector<std::byte>> content, cs::string_sz content_type);

private:
    void* const m_responseObject;
};



// --------------------------------------------------------------------------
// VirtualFileMappingHandler
// 
// a class whose subclasses can serve content for the life of the object;
// see KeyBasedVirtualFileMappingHandler for another version of this handler
// --------------------------------------------------------------------------

class VirtualFileMappingHandler
{
    friend class LocalFileServer;
    friend class PortableLocalFileServer;

public:
    virtual ~VirtualFileMappingHandler() { }

    const std::string& GetUrl() const
    {
        ASSERT(m_virtualFileMapping != nullptr);
        return m_virtualFileMapping->GetUrl();
    }

    // subclasses should call VirtualFileMappingResponse::SetContent
    // with the appropriate content and content type, and return true;
    // returning false will lead to a 404 error
    virtual bool ServeContent(VirtualFileMappingResponse& response) = 0;    

private:
    std::unique_ptr<VirtualFileMapping> m_virtualFileMapping;
};



// --------------------------------------------------------------------------
// FourZeroFourVirtualFileMappingHandler
// 
// a subclass of VirtualFileMappingHandler that can be used when no content
// is available, as it results in a 404 error
// --------------------------------------------------------------------------

class FourZeroFourVirtualFileMappingHandler : public VirtualFileMappingHandler
{
public:
    bool ServeContent(VirtualFileMappingResponse& /*response*/) override
    {
        return false;
    }
};



// --------------------------------------------------------------------------
// CallbackVirtualFileMappingHandler
// 
// a subclass of VirtualFileMappingHandler that serves data using a callback
// function
// --------------------------------------------------------------------------

class CallbackVirtualFileMappingHandler : public VirtualFileMappingHandler
{
public:
    CallbackVirtualFileMappingHandler(std::function<bool(VirtualFileMappingResponse&)> serve_content_callback)
        :   m_serveContentCallback(std::move(serve_content_callback))
    {
        ASSERT(m_serveContentCallback);
    }

    bool ServeContent(VirtualFileMappingResponse& response) override
    {
        return m_serveContentCallback(response);
    }

private:
    std::function<bool(VirtualFileMappingResponse&)> m_serveContentCallback;
};



// --------------------------------------------------------------------------
// DataVirtualFileMappingHandler
// 
// a subclass of VirtualFileMappingHandler that serves data stored in an
// an object that has data and size methods
// --------------------------------------------------------------------------

template<typename StorageType>
class DataVirtualFileMappingHandler : public VirtualFileMappingHandler
{
public:
    DataVirtualFileMappingHandler(StorageType data, std::string content_type)
        :   m_data(std::move(data)),
            m_contentType(std::move(content_type))
    {
        ASSERT(GetPointer(m_data) != nullptr);
    }

    bool ServeContent(VirtualFileMappingResponse& response) override
    {
        if constexpr(std::is_same_v<StorageType, std::shared_ptr<const std::vector<std::byte>>>)
        {
            response.SetContent(m_data, m_contentType);
        }

        else
        {
            const auto* const data = GetPointer(m_data);
            response.SetContent(data->data(), data->size(), m_contentType);
        }

        return true;
    }

private:
    const StorageType m_data;
    const std::string m_contentType;
};



// --------------------------------------------------------------------------
// TextVirtualFileMappingHandler
// 
// a subclass of VirtualFileMappingHandler that serves text in UTF-8 format,
// only converting the text as necessary
//
// defined in VirtualFileMappingHandlers.cpp
// --------------------------------------------------------------------------

class ZHTML_API TextVirtualFileMappingHandler : public VirtualFileMappingHandler
{
public:
    TextVirtualFileMappingHandler(SharableString text, std::string content_type = "text/plain;charset=UTF-8");

    bool ServeContent(VirtualFileMappingResponse& response) override;

private:
    const SharableString m_text;
    const std::string m_contentType;
};



// --------------------------------------------------------------------------
// FileVirtualFileMappingHandler
// 
// a subclass of VirtualFileMappingHandler that serves a file on demand,
// potentially caching the contents
//
// defined in VirtualFileMappingHandlers.cpp
// --------------------------------------------------------------------------

class ZHTML_API FileVirtualFileMappingHandler : public VirtualFileMappingHandler
{
public:
    FileVirtualFileMappingHandler(std::string path, bool cache_contents_on_load, std::string content_type);
    FileVirtualFileMappingHandler(const std::string& path, bool cache_contents_on_load);

    bool ServeContent(VirtualFileMappingResponse& response) override;

private:
    const std::string m_path;
    const std::string m_contentType;
    const bool m_cacheContentsOnLoad;
    std::shared_ptr<const std::vector<std::byte>> m_cachedContent;
};


// --------------------------------------------------------------------------
// KeyBasedVirtualFileMappingHandler
// 
// a class whose subclasses can serve content for the life of the object;
// see VirtualFileMappingHandler for another version of this handler
// --------------------------------------------------------------------------

class ZHTML_API KeyBasedVirtualFileMappingHandler
{
    friend class LocalFileServer;
    friend class PortableLocalFileServer;

public:
    virtual ~KeyBasedVirtualFileMappingHandler() { }

    std::string CreateUrl(std::string_view key_sv, bool use_uri_component_escaping = true) const;

    // subclasses should call VirtualFileMappingResponse::SetContent
    // with the appropriate content and content type, and return true;
    // returning false will lead to a 404 error
    virtual bool ServeContent(VirtualFileMappingResponse& response, const std::string& key) = 0;

protected:
    std::unique_ptr<VirtualFileMapping> m_virtualFileMapping;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline VirtualFileMappingResponse::VirtualFileMappingResponse(void* const response_object)
    :   m_responseObject(response_object)
{
    ASSERT(m_responseObject != nullptr);
}


inline void VirtualFileMappingResponse::SetContent(const std::string_view content_sv, const cs::string_sz content_type)
{
    SetContent(content_sv.data(), content_sv.size(), content_type);
}


inline void VirtualFileMappingResponse::SetContent(const std::vector<std::byte>& content, const cs::string_sz content_type)
{
    SetContent(content.data(), content.size(), content_type);
}


#ifndef WASM
inline void VirtualFileMappingResponse::SetContent(const std::shared_ptr<const std::vector<std::byte>> content, const cs::string_sz content_type)
{
    ASSERT(content != nullptr);
    SetContent(content->data(), content->size(), content_type);
}
#endif
