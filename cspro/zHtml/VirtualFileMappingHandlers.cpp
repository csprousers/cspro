#include "stdafx.h"
#include "VirtualFileMapping.h"


// --------------------------------------------------------------------------
// TextVirtualFileMappingHandler
// --------------------------------------------------------------------------

TextVirtualFileMappingHandler::TextVirtualFileMappingHandler(SharableString text, std::string content_type/* = "text/plain;charset=UTF-8"*/)
    :   m_text(std::move(text)),
        m_contentType(std::move(content_type))
{
}


bool TextVirtualFileMappingHandler::ServeContent(VirtualFileMappingResponse& response)
{
    response.SetContent(*m_text, m_contentType);
    return true;
}



// --------------------------------------------------------------------------
// FileVirtualFileMappingHandler
// --------------------------------------------------------------------------

FileVirtualFileMappingHandler::FileVirtualFileMappingHandler(std::string path, const bool cache_contents_on_load, std::string content_type)
    :   m_path(std::move(path)),
        m_contentType(std::move(content_type)),
        m_cacheContentsOnLoad(cache_contents_on_load)
{
    ASSERT(PortableFunctions::FileIsRegular(m_path));
}


FileVirtualFileMappingHandler::FileVirtualFileMappingHandler(const std::string& path, const bool cache_contents_on_load)
    :   FileVirtualFileMappingHandler(path, cache_contents_on_load, ValueOrDefault(MimeType::GetTypeFromFileExtension(PortableFunctions::PathGetFileExtension(path))))
{
}


bool FileVirtualFileMappingHandler::ServeContent(VirtualFileMappingResponse& response)
{
    if( m_cachedContent != nullptr )
    {
        response.SetContent(m_cachedContent, m_contentType);
        return true;
    }

    try
    {
        std::shared_ptr<const std::vector<std::byte>> content = FileIO::Read(m_path);
        response.SetContent(content, m_contentType);

        if( m_cacheContentsOnLoad )
            m_cachedContent = std::move(content);

        return true;
    }

    catch(...)
    {
        return false;
    }
}



// --------------------------------------------------------------------------
// KeyBasedVirtualFileMappingHandler
// --------------------------------------------------------------------------

std::string KeyBasedVirtualFileMappingHandler::CreateUrl(const std::string_view key_sv, const bool use_uri_component_escaping/* = true*/) const
{
    ASSERT(m_virtualFileMapping != nullptr && !key_sv.empty());

    const std::string escaped_key = use_uri_component_escaping ? Encoders::ToUriComponent(key_sv) :
                                                                 Encoders::ToUri(key_sv);

    return PortableFunctions::PathAppendForwardSlashToPath(m_virtualFileMapping->GetUrl(), escaped_key);
}
