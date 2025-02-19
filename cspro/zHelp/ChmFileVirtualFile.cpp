#include "stdafx.h"
#include "ChmFileVirtualFile.h"
#include "ChmFileReader.h"
#include <zToolsO/Encoders.h>
#include <zUtilO/FileExtensions.h>
#include <zUtilO/MimeType.h>
#include <zHtml/SharedHtmlLocalFileServer.h>


namespace
{
     constexpr size_t MaxCachedContentCount = 50;
}


ChmFileVirtualFile::ChmFileVirtualFile(const std::string& help_file_path)
    :   m_helpName(Path::GetFilenameWithoutExtension(help_file_path)),
        m_helpDirectory(PortableFunctions::PathGetDirectory(help_file_path)),
        m_chmFileReader(std::make_unique<ChmFileReader>(help_file_path)),
        m_fileServer(std::make_unique<SharedHtmlLocalFileServer>())
{
    m_fileServer->CreateVirtualDirectory(*this);
}


ChmFileVirtualFile::~ChmFileVirtualFile()
{
}


std::string ChmFileVirtualFile::GetBaseUrl(const std::string_view help_name_sv) const
{
    ASSERT(help_name_sv == Encoders::ToUriComponent(help_name_sv));

    // the base URL will look something like: http://localhost:53909/vf/1/CSCode
    return PortableFunctions::PathAppendForwardSlashToPath(m_virtualFileMapping->GetUrl(), help_name_sv);
}


std::unique_ptr<UriResolver> ChmFileVirtualFile::GetDefaultTopicUriResolver() const
{
    const std::string& default_topic_path = m_chmFileReader->GetDefaultTopicPath();
    std::string default_topic_url = PortableFunctions::PathAppendForwardSlashToPath(GetBaseUrl(m_helpName), Encoders::ToUriComponent(default_topic_path));

    return UriResolver::CreateUriDomain(std::move(default_topic_url), m_virtualFileMapping->GetUrl(), "help://");
}


bool ChmFileVirtualFile::ServeContent(VirtualFileMappingResponse& response, const std::string& key)
{
    try
    {
        // content will come from...
        const auto [help_name_sv, topic_filename_sv] = SO::GetTextOnEitherSideOfCharacter(key, '/');
        ChmFileReader* chm_file_reader;
        const std::string_view* filename_sv;

        // ...this CHM file
        if( help_name_sv == m_helpName )
        {
            chm_file_reader = m_chmFileReader.get();
            filename_sv = &topic_filename_sv;
        }

        // ...or this CHM file (in a badly formed URL)
        else if( topic_filename_sv.empty() )
        {
            ASSERT(false);
            chm_file_reader = m_chmFileReader.get();
            filename_sv = &help_name_sv;
        }

        // ...or from another CHM file
        else
        {
            const auto& lookup = m_externalChmFileReaders.find(help_name_sv);

            if( lookup != m_externalChmFileReaders.cend() )
            {
                chm_file_reader = &lookup->second;
            }

            else
            {
                const std::string help_file_path = PortableFunctions::CreateFilePath(m_helpDirectory, help_name_sv, FileExtensions::CHM);
                chm_file_reader = &m_externalChmFileReaders.try_emplace(std::string(help_name_sv), ChmFileReader(help_file_path)).first->second;
            }

            filename_sv = &topic_filename_sv;
        }

        response.SetContent(GetContent(*chm_file_reader, std::string(*filename_sv)),
                            ValueOrDefault(MimeType::GetServerTypeFromFileExtension(PortableFunctions::PathGetFileExtension(key))));

        return true;
    }

    catch(...)
    {
        return false;
    }
}


std::shared_ptr<const std::vector<std::byte>> ChmFileVirtualFile::GetContent(ChmFileReader& chm_file_reader, const std::string& filename)
{
    // see if the content has already been retrieved
    const auto& lookup = std::find_if(m_cachedContent.cbegin(), m_cachedContent.cend(),
        [&](const auto& cached_content)
        {
            return ( std::get<0>(cached_content) == &chm_file_reader &&
                     SO::EqualsNoCase(std::get<1>(cached_content), filename) );
        });

    if( lookup != m_cachedContent.cend() )
        return std::get<2>(*lookup);

    // if not, read and cache the content
    std::vector<std::byte> content;

    {
        std::lock_guard<std::mutex> lock(m_accessMutex);
        content = chm_file_reader.ReadObject(filename);
    }

    if( FileExtensions::IsFileHtml(filename) )
        PreprocessHtmlContent(content);

    // make sure not too many things are cached
    if( m_cachedContent.size() == MaxCachedContentCount )
        m_cachedContent.erase(m_cachedContent.cbegin());

    return std::get<2>(m_cachedContent.emplace_back(&chm_file_reader,
                                                     filename,
                                                     std::make_unique<const std::vector<std::byte>>(std::move(content))));
}


void ChmFileVirtualFile::PreprocessHtmlContent(std::vector<std::byte>& content)
{
    // adjust links to other help files as necessary
    constexpr std::string_view LinkSearchText_sv = ".chm::";
    constexpr std::string_view HrefStartText_sv = "href=";

    if( std::string_view(reinterpret_cast<const char*>(content.data()), content.size()).find(LinkSearchText_sv) == std::string_view::npos )
        return;

    std::string html(reinterpret_cast<const char*>(content.data()), content.size());

    for( size_t link_search_text_pos = 0; ( link_search_text_pos = html.find(LinkSearchText_sv, link_search_text_pos) ) != std::string::npos; )
    {
        const size_t initial_html_length = html.length();

        // make sure that this is part of a link
        size_t quote_start_pos = link_search_text_pos - 1;

        while( quote_start_pos < html.size() && html[quote_start_pos] != '"' )
            --quote_start_pos;

        if( quote_start_pos >= HrefStartText_sv.length() && quote_start_pos < html.size() &&
            SO::StartsWith(std::string_view(html).substr(quote_start_pos - HrefStartText_sv.length()), HrefStartText_sv) )
        {
            // links will appear as: csconcat.chm::introduction_to_concatenate_data.html
            // replace to: [local file server]/csconcat/introduction_to_concatenate_data.html
            std::string help_name = html.substr(quote_start_pos + 1, link_search_text_pos - quote_start_pos - 1);

            const std::string base_url = PortableFunctions::PathEnsureTrailingForwardSlash(GetBaseUrl(help_name));

            html.replace(quote_start_pos + 1, help_name.length() + LinkSearchText_sv.length(), base_url);
        }

        const size_t html_increase_length = html.length() - initial_html_length;

        link_search_text_pos += ( html_increase_length > 0 ) ? initial_html_length :
                                                               ReturnProgrammingError(0);
    }

    // replace the content with the modified content
    content.resize(html.size());
    memcpy(content.data(), html.data(), html.size());
}
