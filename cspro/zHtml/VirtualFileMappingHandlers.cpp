#include "stdafx.h"
#include "VirtualFileMapping.h"
#include <zToolsO/FileIO.h>


// --------------------------------------------------------------------------
// TextVirtualFileMappingHandler
// --------------------------------------------------------------------------

TextVirtualFileMappingHandler::TextVirtualFileMappingHandler(TextStorage text, std::string content_type/* = "text/plain;charset=UTF-8"*/)
    :   m_text(std::move(text)),
        m_contentType(std::move(content_type))
{
}


TextVirtualFileMappingHandler::TextVirtualFileMappingHandler(TextStorage text, wstring_view content_type_sv)
    :   TextVirtualFileMappingHandler(std::move(text), UTF8Convert::WideToUTF8(content_type_sv))
{
}


bool TextVirtualFileMappingHandler::ServeContent(void* response_object)
{
    if( std::holds_alternative<std::string>(m_text) )
    {
        const std::string& this_text = std::get<std::string>(m_text);
        LocalFileServerSetResponse(response_object, this_text, m_contentType);
    }

    else
    {
        // if a wide string, convert to UTF-8 bytes
        if( std::holds_alternative<std::wstring>(m_text) )
        {
            const std::wstring& this_text = std::get<std::wstring>(m_text);
            m_text = UTF8Convert::WideToUTF8Buffer(this_text);
        }

        ASSERT(std::holds_alternative<std::vector<std::byte>>(m_text));

        const std::vector<std::byte>& content = std::get<std::vector<std::byte>>(m_text);
        LocalFileServerSetResponse(response_object, content, m_contentType);
    }

    return true;
}



// --------------------------------------------------------------------------
// FileVirtualFileMappingHandler
// --------------------------------------------------------------------------

FileVirtualFileMappingHandler::FileVirtualFileMappingHandler(std::wstring path, bool cache_contents_on_load, wstring_view content_type_sv)
    :   m_path(std::move(path)),
        m_contentType(UTF8Convert::WideToUTF8(content_type_sv)),
        m_cacheContentsOnLoad(cache_contents_on_load)
{
    ASSERT(PortableFunctions::FileIsRegular(m_path));
}


FileVirtualFileMappingHandler::FileVirtualFileMappingHandler(const std::wstring& path, bool cache_contents_on_load)
    :   FileVirtualFileMappingHandler(path, cache_contents_on_load, ValueOrDefault(MimeType::GetTypeFromFileExtension(PortableFunctions::PathGetFileExtension(path))))
{
}


bool FileVirtualFileMappingHandler::ServeContent(void* response_object)
{
    if( m_cachedContent != nullptr )
    {
        LocalFileServerSetResponse(response_object, *m_cachedContent, m_contentType);
        return true;
    }

    try
    {
        std::unique_ptr<const std::vector<std::byte>> content = FileIO::Read(m_path);
        LocalFileServerSetResponse(response_object, *content, m_contentType);

        if( m_cacheContentsOnLoad )
            m_cachedContent = std::move(content);

        return true;
    }

    catch(...)
    {
        return false;
    }
}
