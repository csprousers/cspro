#pragma once

#include <zHtml/zHtml.h>
#include <zToolsO/Encoders.h>
#include <zToolsO/PortableFunctions.h>
#include <zToolsO/Utf8Convert.h>


// --------------------------------------------------------------------------
// VirtualFileMapping
// 
// an object that controls the lifecycle of a single virtual file mapping
// --------------------------------------------------------------------------

class VirtualFileMapping
{
    friend class LocalFileServer;
    friend class SharedHtmlLocalFileServer;

private:
    VirtualFileMapping(std::wstring url, std::shared_ptr<bool> mapping_active)
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

    const std::wstring& GetUrl() { return m_url; }

private:
    std::wstring m_url;
    std::shared_ptr<bool> m_mappingActive;
};



// --------------------------------------------------------------------------
// LocalFileServerSetResponse
//
// a function that the virtual file mappers can use to return data without
// needing to know details about the local file server
// --------------------------------------------------------------------------

ZHTML_API void LocalFileServerSetResponse(void* response_object, const void* content_data, size_t content_size,
                                          const char* content_type);

inline void LocalFileServerSetResponse(void* response_object, const void* content_data, size_t content_size, const std::string& content_type)
{
    LocalFileServerSetResponse(response_object, content_data, content_size, content_type.c_str());
}

inline void LocalFileServerSetResponse(void* response_object, const void* content_data, size_t content_size, wstring_view content_type_sv)
{
    LocalFileServerSetResponse(response_object, content_data, content_size, UTF8Convert::WideToUTF8(content_type_sv).c_str());
}

template<typename ContentType>
void LocalFileServerSetResponse(void* response_object, const std::vector<std::byte>& content, ContentType&& content_type)
{
    LocalFileServerSetResponse(response_object, content.data(), content.size(), std::forward<ContentType>(content_type));
}

template<typename ContentType>
void LocalFileServerSetResponse(void* response_object, const std::string& content, ContentType&& content_type)
{
    LocalFileServerSetResponse(response_object, content.data(), content.size(), std::forward<ContentType>(content_type));
}



// --------------------------------------------------------------------------
// VirtualFileMappingHandler
// 
// a class whose subclasses can serve content for the life of the object;
// see KeyBasedVirtualFileMappingHandler for another version of this handler
// --------------------------------------------------------------------------

class VirtualFileMappingHandler
{
    friend class LocalFileServer;

public:
    virtual ~VirtualFileMappingHandler() { }

    const std::wstring& GetUrl() const
    {
        ASSERT(m_virtualFileMapping != nullptr);
        return m_virtualFileMapping->GetUrl();
    }

    // subclasses should call LocalFileServerSetResponse, passing the response object,
    // with the appropriate content and content type, and return true;
    // returning false will lead to a 404 error
    virtual bool ServeContent(void* response_object) = 0;    

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
    bool ServeContent(void* /*response_object*/) override
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
    CallbackVirtualFileMappingHandler(std::function<bool(void*)> serve_content_callback)
        :   m_serveContentCallback(std::move(serve_content_callback))
    {
        ASSERT(m_serveContentCallback);
    }

    bool ServeContent(void* response_object) override
    {
        return m_serveContentCallback(response_object);
    }

private:
    std::function<bool(void*)> m_serveContentCallback;
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
    }

    DataVirtualFileMappingHandler(StorageType data, wstring_view content_type_sv)
        :   DataVirtualFileMappingHandler(std::move(data), UTF8Convert::WideToUTF8(content_type_sv))
    {
    }

    bool ServeContent(void* response_object) override
    {
        if constexpr(IsPointer<StorageType>())
        {
            LocalFileServerSetResponse(response_object, m_data->data(), m_data->size(), m_contentType.c_str());
        }

        else
        {
            LocalFileServerSetResponse(response_object, m_data.data(), m_data.size(), m_contentType.c_str());
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
    using TextStorage = std::variant<std::string, std::wstring, std::vector<std::byte>>;

    TextVirtualFileMappingHandler(TextStorage text, std::string content_type = "text/plain;charset=UTF-8");
    TextVirtualFileMappingHandler(TextStorage text, wstring_view content_type_sv);

    bool ServeContent(void* response_object) override;

private:
    TextStorage m_text;
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
    FileVirtualFileMappingHandler(std::wstring path, bool cache_contents_on_load, wstring_view content_type_sv);
    FileVirtualFileMappingHandler(const std::wstring& path, bool cache_contents_on_load);

    bool ServeContent(void* response_object) override;

private:
    const std::wstring m_path;
    const std::string m_contentType;
    const bool m_cacheContentsOnLoad;
    std::unique_ptr<const std::vector<std::byte>> m_cachedContent;
};


// --------------------------------------------------------------------------
// KeyBasedVirtualFileMappingHandler
// 
// a class whose subclasses can serve content for the life of the object;
// see VirtualFileMappingHandler for another version of this handler
// --------------------------------------------------------------------------

class KeyBasedVirtualFileMappingHandler
{
    friend class LocalFileServer;

public:
    virtual ~KeyBasedVirtualFileMappingHandler() { }

    std::wstring CreateUrl(const wstring_view key_sv, const bool use_uri_component_escaping = true) const
    {
        ASSERT(m_virtualFileMapping != nullptr && !key_sv.empty());

        const std::wstring escaped_key = use_uri_component_escaping ? Encoders::ToUriComponent(key_sv) :
                                                                      Encoders::ToUri(key_sv);

        return PortableFunctions::PathAppendForwardSlashToPath(m_virtualFileMapping->GetUrl(), escaped_key);
    }

    // subclasses should call LocalFileServerSetResponse, passing the response object,
    // with the appropriate content and content type, and return true;
    // returning false will lead to a 404 error
    virtual bool ServeContent(void* response_object, const std::wstring& key) = 0;

protected:
    std::unique_ptr<VirtualFileMapping> m_virtualFileMapping;
};
