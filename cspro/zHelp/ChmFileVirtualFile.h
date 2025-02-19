#pragma once

#include <zHelp/zHelp.h>
#include <zHtml/UriResolver.h>
#include <zHtml/VirtualFileMapping.h>
#include <zToolsO/CaseInsensitiveComparer.h>
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
    ChmFileVirtualFile(const std::string& help_file_path);
    ~ChmFileVirtualFile();

    std::unique_ptr<UriResolver> GetDefaultTopicUriResolver() const;

private:
    bool ServeContent(VirtualFileMappingResponse& response, const std::string& key) override;

private:
    std::string GetBaseUrl(std::string_view help_name_sv) const;

    std::shared_ptr<const std::vector<std::byte>> GetContent(ChmFileReader& chm_file_reader, const std::string& filename);

    void PreprocessHtmlContent(std::vector<std::byte>& content);

private:
    std::string m_helpName;
    std::string m_helpDirectory;

    std::unique_ptr<ChmFileReader> m_chmFileReader;
    std::map<std::string, ChmFileReader, cs::case_insensitive_less> m_externalChmFileReaders;

    std::unique_ptr<SharedHtmlLocalFileServer> m_fileServer;

    std::mutex m_accessMutex;
    std::vector<std::tuple<const ChmFileReader*, std::string, std::shared_ptr<const std::vector<std::byte>>>> m_cachedContent;
};
