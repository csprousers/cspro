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

    std::string CreateFileUrl(const std::string& file_path, bool get_complete_escaped_url, bool make_url_unique = false);
    std::string CreateUniqueFileUrl(const std::string& file_path);

    VirtualFileMapping CreateVirtualHtmlFile(const std::string& directory, std::function<SharableString()> callback);

private:
    LocalFileServer m_localFileServer;
    std::map<std::string, std::string> m_volumeMapping;
    std::set<std::string> m_virtualHtmlFilePaths;
};


SharedHtmlLocalFileServer::Impl::Impl()
    :   m_localFileServer(Html::GetDirectory())
{
    m_localFileServer.Start();
}


std::string SharedHtmlLocalFileServer::Impl::CreateFileUrl(const std::string& file_path, const bool get_complete_escaped_url, const bool make_url_unique/* = false*/)
{
    const std::string real_volume_name = PathGetVolume(file_path);
    const std::string path_without_volume = PortableFunctions::PathToForwardSlash(file_path.substr(real_volume_name.length()));

    cs::non_null_shared_or_raw_ptr<const std::string> mapped_volume_name(&real_volume_name);

    if( make_url_unique )
        mapped_volume_name = std::make_shared<std::string>(IntToString(m_volumeMapping.size()) + real_volume_name);

    const auto& volume_lookup = m_volumeMapping.find(*mapped_volume_name);
    std::string volume_url;

    if( volume_lookup != m_volumeMapping.cend() )
    {
        volume_url = volume_lookup->second;
    }

    else
    {
        // if the volume has not been mapped yet, mount it,
        // removing any non-alphanumeric characters from the volume name
        std::string volume_letter;

        for( const char ch : *mapped_volume_name )
        {
            if( std::isalnum(ch) )
                volume_letter.push_back(ch);
        }

        volume_url = FormatText("/%s/%s/", LocalhostUrl::LocalFileSystemDirectoryName, volume_letter.c_str());

        m_localFileServer.AddMountPoint(volume_url, real_volume_name);

        m_volumeMapping.try_emplace(*mapped_volume_name, volume_url);
    }

    ASSERT(!volume_url.empty() && volume_url.front() == '/' && volume_url.back() == '/');

    if( get_complete_escaped_url )
    {
        return SO::Concatenate(m_localFileServer.GetBaseUrl(),
                               std::string_view(volume_url).substr(1),
                               Encoders::ToUri(path_without_volume, false));
    }

    else
    {
        return volume_url + path_without_volume;
    }
}


std::string SharedHtmlLocalFileServer::Impl::CreateUniqueFileUrl(const std::string& file_path)
{
    return CreateFileUrl(file_path, true, true);
}


VirtualFileMapping SharedHtmlLocalFileServer::Impl::CreateVirtualHtmlFile(const std::string& directory, std::function<SharableString()> callback)
{
    // create a file path that does not exist in the directory and that was not used for a previous mapping
    const std::string file_path = PortableFunctions::GetUniqueFilePathInDirectory(directory, FileExtensions::HTML, nullptr,
        [&](const std::string& test_file_path)
        {
            return ( m_virtualHtmlFilePaths.find(test_file_path) == m_virtualHtmlFilePaths.cend() );
        });

    m_virtualHtmlFilePaths.insert(file_path);

    const std::string url_for_mapping = CreateFileUrl(file_path, false);

    std::shared_ptr<bool> virtual_file_mapping_active = m_localFileServer.AddMapping(url_for_mapping, "text/html; charset=utf-8", std::move(callback));

    return VirtualFileMapping(CreateFileUrl(file_path, true), std::move(virtual_file_mapping_active));
}



// --------------------------------------------------------------------------
// SharedHtmlLocalFileServer
// --------------------------------------------------------------------------

std::unique_ptr<SharedHtmlLocalFileServer::Impl> SharedHtmlLocalFileServer::m_sharedImpl;


SharedHtmlLocalFileServer::SharedHtmlLocalFileServer(std::string project_root/* = std::string()*/)
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
    ApplicationShutdownRunner* const application_shutdown_runner = ApplicationShutdownRunner::Get();

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
    const std::vector<std::string> automatically_mapped_drives = LocalhostSettings::GetDrivesToAutomaticallyMap();

    if( !automatically_mapped_drives.empty() )
    {
        const std::vector<std::string> logical_drives = GetLogicalDrivesVector();

        for( const std::string& drive : automatically_mapped_drives )
        {
            if( std::find(logical_drives.cbegin(), logical_drives.cend(), drive) != logical_drives.cend() )
            {
                const std::string fake_filename = Path::Combine(drive, "j");
                CreateFileUrl(fake_filename);
            }
        }
    }
}


SharedHtmlLocalFileServer::~SharedHtmlLocalFileServer()
{
    if( !m_usingSharedImpl )
        delete m_impl;
}


std::string SharedHtmlLocalFileServer::CreateProjectUrl(const std::string_view url_from_project_root_sv) const
{
    ASSERT(!url_from_project_root_sv.empty() || url_from_project_root_sv.front() != '/');

    return SO::Concatenate(m_impl->GetLocalFileServer().GetBaseUrl(),
                           m_projectRoot,
                           url_from_project_root_sv);
}


std::string SharedHtmlLocalFileServer::CreateFileUrl(const std::string& file_path)
{
    return m_impl->CreateFileUrl(file_path, true);
}


std::string SharedHtmlLocalFileServer::CreateUniqueFileUrl(const std::string& file_path)
{
    return m_impl->CreateUniqueFileUrl(file_path);
}


VirtualFileMapping SharedHtmlLocalFileServer::CreateVirtualHtmlFile(const std::string& directory, std::function<SharableString()> callback)
{
    return m_impl->CreateVirtualHtmlFile(directory, std::move(callback));
}


void SharedHtmlLocalFileServer::CreateVirtualFile(VirtualFileMappingHandler& virtual_file_mapping_handler, const cs::string_sz filename/* = ""*/)
{
    m_impl->GetLocalFileServer().AddMapping(virtual_file_mapping_handler, filename);
}


void SharedHtmlLocalFileServer::CreateVirtualDirectory(KeyBasedVirtualFileMappingHandler& key_based_virtual_file_mapping_handler)
{
    m_impl->GetLocalFileServer().AddMapping(key_based_virtual_file_mapping_handler);
}
