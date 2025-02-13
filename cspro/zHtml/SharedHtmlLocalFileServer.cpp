#include "stdafx.h"
#include "SharedHtmlLocalFileServer.h"
#include "LocalFileServer.h"
#include "LocalhostSettings.h"
#include "LocalhostUrl.h"
#include <zUtilF/ApplicationShutdownRunner.h>
#include <mutex>


// --------------------------------------------------------------------------
// SharedHtmlLocalFileServer::Impl
// --------------------------------------------------------------------------

class SharedHtmlLocalFileServer::Impl
{
public:
    Impl();

    LocalFileServer& GetLocalFileServer() { return m_localFileServer; }

    std::wstring GetFilenameUrl(NullTerminatedStringView filename, bool get_complete_escaped_url);

    VirtualFileMapping CreateVirtualHtmlFile(wstring_view directory_sv, std::function<std::string()> callback);

private:
    LocalFileServer m_localFileServer;
    std::map<std::wstring, std::wstring> m_volumeMapping;
    std::set<std::wstring> m_virtualHtmlFilenames;
};


SharedHtmlLocalFileServer::Impl::Impl()
    :   m_localFileServer(Html::GetDirectory())
{
    m_localFileServer.Start();
}


std::wstring SharedHtmlLocalFileServer::Impl::GetFilenameUrl(const NullTerminatedStringView filename, const bool get_complete_escaped_url)
{
    const std::wstring volume_name = PathGetVolume(filename);

    std::wstring path_without_volume = PortableFunctions::PathToForwardSlash<std::wstring>(filename.substr(volume_name.length()));

    const auto& volume_lookup = m_volumeMapping.find(volume_name);
    std::wstring volume_url;

    if( volume_lookup != m_volumeMapping.cend() )
    {
        volume_url = volume_lookup->second;
    }

    else
    {
        // if the volume has not been mapped yet, mount it,
        // removing any non-letter characters from the volume name
        std::wstring volume_letter;

        for( const TCHAR ch : volume_name )
        {
            if( std::isalpha(ch) )
                volume_letter.push_back(ch);
        }

        volume_url = FormatTextCS2WS(_T("/%s/%s/"), LocalhostUrl::LocalFileSystemDirectoryName, volume_letter.c_str());

        m_localFileServer.AddMountPoint(volume_url, volume_name);

        m_volumeMapping.try_emplace(volume_name, volume_url);
    }

    ASSERT(!volume_url.empty() && volume_url.front() == '/' && volume_url.back() == '/');

    if( get_complete_escaped_url )
    {
        return SO::Concatenate(m_localFileServer.GetBaseUrl(), wstring_view(volume_url).substr(1), Encoders::ToUri(path_without_volume, false));
    }

    else
    {
        return SO::Concatenate(std::move(volume_url), std::move(path_without_volume));
    }
}


VirtualFileMapping SharedHtmlLocalFileServer::Impl::CreateVirtualHtmlFile(const wstring_view directory_sv, std::function<std::string()> callback)
{
    // create a filename that does not exist in the directory and that was not used for a previous mapping
    const std::wstring filename = PortableFunctions::GetUniqueFilenameInDirectory(directory_sv, FileExtensions::HTML, nullptr,
        [&](const std::wstring& test_filename)
        {
            return ( m_virtualHtmlFilenames.find(test_filename) == m_virtualHtmlFilenames.cend() );
        });

    m_virtualHtmlFilenames.insert(filename);

    const std::wstring url_for_mapping = GetFilenameUrl(filename, false);

    std::shared_ptr<bool> virtual_file_mapping_active = m_localFileServer.AddMapping(url_for_mapping, "text/html; charset=utf-8", std::move(callback));

    return VirtualFileMapping(GetFilenameUrl(filename, true), std::move(virtual_file_mapping_active));
}



// --------------------------------------------------------------------------
// SharedHtmlLocalFileServer
// --------------------------------------------------------------------------

std::unique_ptr<SharedHtmlLocalFileServer::Impl> SharedHtmlLocalFileServer::m_sharedImpl;


SharedHtmlLocalFileServer::SharedHtmlLocalFileServer(std::wstring project_root/* = std::wstring()*/)
    :   m_projectRoot(PortableFunctions::PathEnsureTrailingForwardSlash(std::move(project_root))),
        m_impl(m_sharedImpl.get()),
        m_usingSharedImpl(true)
{
    ASSERT(m_projectRoot == Encoders::ToUri(m_projectRoot, false));

    // set up the local file server only if a shared one has not already been set up
    if( m_impl != nullptr )
        return;

    static std::mutex creation_mutex;
    std::lock_guard<std::mutex> creation_lock(creation_mutex);

    m_impl = new SharedHtmlLocalFileServer::Impl();

    // if there is no application shutdown runner in place, we cannot use a shared implementation
    // because we need a way to be able to stop the server thread when using the shared implementation
    ApplicationShutdownRunner* application_shutdown_runner = ApplicationShutdownRunner::Get();

    if( application_shutdown_runner == nullptr )
    {
        m_usingSharedImpl = false;
    }

    else
    {
        m_sharedImpl.reset(m_impl);

        application_shutdown_runner->AddShutdownOperation([]()
        {
            m_sharedImpl.reset();
        });
    }

    // potentially map some drives automatically
    const std::vector<std::wstring> automatically_mapped_drives = LocalhostSettings::GetDrivesToAutomaticallyMap();

    if( !automatically_mapped_drives.empty() )
    {
        const std::vector<std::wstring> logical_drives = GetLogicalDrivesVector();

        for( const std::wstring& drive : automatically_mapped_drives )
        {
            if( std::find(logical_drives.cbegin(), logical_drives.cend(), drive) != logical_drives.cend() )
            {
                const std::wstring fake_filename = PortableFunctions::PathAppendToPath(drive, _T("j"));
                GetFilenameUrl(fake_filename);
            }
        }
    }
}


SharedHtmlLocalFileServer::~SharedHtmlLocalFileServer()
{
    if( !m_usingSharedImpl )
        delete m_impl;
}


std::wstring SharedHtmlLocalFileServer::GetProjectUrl(const wstring_view url_from_project_root_sv) const
{
    ASSERT(!url_from_project_root_sv.empty() || url_from_project_root_sv.front() != '/');
    return SO::Concatenate(m_impl->GetLocalFileServer().GetBaseUrl(), m_projectRoot, url_from_project_root_sv);
}


std::wstring SharedHtmlLocalFileServer::GetFilenameUrl(const NullTerminatedStringView filename)
{
    return m_impl->GetFilenameUrl(filename, true);
}


VirtualFileMapping SharedHtmlLocalFileServer::CreateVirtualHtmlFile(const wstring_view directory_sv, std::function<std::string()> callback)
{
    return m_impl->CreateVirtualHtmlFile(directory_sv, std::move(callback));
}


void SharedHtmlLocalFileServer::CreateVirtualFile(VirtualFileMappingHandler& virtual_file_mapping_handler, const NullTerminatedString filename/* = _T("")*/)
{
    m_impl->GetLocalFileServer().AddMapping(virtual_file_mapping_handler, filename);
}


void SharedHtmlLocalFileServer::CreateVirtualDirectory(KeyBasedVirtualFileMappingHandler& key_based_virtual_file_mapping_handler)
{
    m_impl->GetLocalFileServer().AddMapping(key_based_virtual_file_mapping_handler);
}
