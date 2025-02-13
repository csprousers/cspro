#pragma once

#include <zHelp/zHelp.h>
#include <zHtml/UriResolver.h>
#include <zHtml/VirtualFileMapping.h>
#include <mutex>

class ChmFileReader;
class SharedHtmlLocalFileServer;


// --------------------------------------------------------------------------
// ChmFileVirtualFile: a virtual file mapping that wraps a CHM file
//
// creation issues will result in CSProException exceptions
// --------------------------------------------------------------------------

class ZHELP_API ChmFileVirtualFile : private KeyBasedVirtualFileMappingHandler
{
public:
    ChmFileVirtualFile(const std::wstring& filename);
    ~ChmFileVirtualFile();

    std::unique_ptr<UriResolver> GetDefaultTopicUriResolver() const;

private:
    bool ServeContent(void* response_object, const std::wstring& key) override;

private:
    std::wstring GetBaseUrl(const std::wstring& help_name) const;

    std::shared_ptr<std::vector<std::byte>> GetContent(ChmFileReader& chm_file_reader, StringNoCase filename);

    void PreprocessHtmlContent(std::vector<std::byte>& content);

private:
    std::wstring m_helpName;
    std::wstring m_helpDirectory;

    std::unique_ptr<ChmFileReader> m_chmFileReader;
    std::map<StringNoCase, ChmFileReader> m_externalChmFileReaders;

    std::unique_ptr<SharedHtmlLocalFileServer> m_fileServer;

    std::mutex m_accessMutex;
    std::vector<std::tuple<const ChmFileReader*, StringNoCase, std::shared_ptr<std::vector<std::byte>>>> m_cachedContent;
};
