#include "stdafx.h"
#include "ApplicationPackageManager.h"
#include "JsonConverter.h"
#include <zToolsO/FileIO.h>


ApplicationPackageManager::ApplicationPackageManager(std::string root_directory)
    :   m_rootDirectory(PortableFunctions::PathEnsureTrailingSlash(std::move(root_directory)))
{
    ASSERT(PortableFunctions::FileIsDirectory(m_rootDirectory));
}


std::optional<std::string> ApplicationPackageManager::GetInstalledPackageJsonFromSpecFile(const std::string& package_spec_file_path)
{
    if( PortableFunctions::FileIsRegular(package_spec_file_path) )
    {
        try
        {
            return FileIO::ReadText(package_spec_file_path);
        }

        catch( const std::exception& exception )
        {
            SYNCLOG_WARNING << "Failed to read installed package JSON. Ignoring existing install. " << exception.what();
        }
    }

    return std::nullopt;
}


std::optional<std::string> ApplicationPackageManager::GetPackageJsonFromInstalledDirectory(const std::string& installed_package_directory)
{
    std::string package_spec_file_path = Path::Combine(installed_package_directory, "package.json");

    // Older packages will have .csds instead of .json
    if( !PortableFunctions::FileExists(package_spec_file_path) )
        package_spec_file_path = PortableFunctions::PathReplaceFileExtension(package_spec_file_path, FileExtensions::DeploySpec);

    return GetInstalledPackageJsonFromSpecFile(package_spec_file_path);
}


std::optional<ApplicationPackage> ApplicationPackageManager::ParsePackageSpec(const std::string& package_spec_json)
{
    try
    {
        return JsonConverter::CreateApplicationPackageFromJson(Json::Parse(package_spec_json));
    }

    catch( const JsonParseException& exception )
    {
        SYNCLOG_WARNING << "Failed to parse installed package JSON. Ignoring existing install. " << exception.what();
    }

    return std::nullopt;
}


std::optional<ApplicationPackage> ApplicationPackageManager::GetInstalledPackageFromSpecFile(const std::string& package_spec_file_path)
{
    std::optional<std::string> package_spec_json = GetInstalledPackageJsonFromSpecFile(package_spec_file_path);

    if( package_spec_json.has_value() )
    {
        std::optional<ApplicationPackage> package = ParsePackageSpec(*package_spec_json);

        if( package.has_value() )
            package->SetInstallPath(PortableFunctions::PathGetDirectory(package_spec_file_path));

        return package;
    }

    return std::nullopt;
}


std::string ApplicationPackageManager::GetPackageInstallDirectory(const std::string& package_name) const
{
    return Path::Combine(m_rootDirectory, ReplaceInvalidFileChars(package_name, '_'));
}


std::string ApplicationPackageManager::GetPackageDownloadDirectory() const
{
    return Path::Combine(m_rootDirectory, "Packages");
}


std::unique_ptr<ApplicationPackageManager::ApplicationWithSignature> ApplicationPackageManager::GetApplicationPackageWithSignatureFromInstalledDirectory(const std::string& installed_package_directory)
{
    const std::optional<std::string> installed_package_json = GetPackageJsonFromInstalledDirectory(installed_package_directory);

    if( installed_package_json.has_value() )
    {
        std::optional<ApplicationPackage> package = ParsePackageSpec(*installed_package_json);

        if( package.has_value() )
        {
            package->SetInstallPath(installed_package_directory);

            return std::make_unique<ApplicationWithSignature>(ApplicationWithSignature { std::move(*package), Hash::Md5::Create(*installed_package_json) });
        }
    }

    return nullptr;
}


std::unique_ptr<ApplicationPackageManager::ApplicationWithSignature> ApplicationPackageManager::GetInstalledApplicationPackageWithSignature(const std::string& package_name) const
{
    return GetApplicationPackageWithSignatureFromInstalledDirectory(GetPackageInstallDirectory(package_name));
}


std::unique_ptr<ApplicationPackageManager::ApplicationWithSignature> ApplicationPackageManager::GetApplicationPackageWithSignatureFromApplicationDirectory(const std::string& application_file_path) const
{
    ASSERT(application_file_path == PortableFunctions::PathToNativeSlash(application_file_path));
    ASSERT(m_rootDirectory == PortableFunctions::PathEnsureTrailingSlash(m_rootDirectory));

    if( SO::StartsWithNoCase(application_file_path, m_rootDirectory) )
    {
        const size_t first_slash_pos = application_file_path.find(Path::NativeSlashChar, m_rootDirectory.length());

        if( first_slash_pos != std::string::npos )
        {
            const std::string& installed_package_directory = application_file_path.substr(0, first_slash_pos + 1);
            return GetApplicationPackageWithSignatureFromInstalledDirectory(installed_package_directory);
        }
    }

    return nullptr;
}


std::vector<ApplicationPackage> ApplicationPackageManager::GetInstalledApplications() const
{
    std::vector<ApplicationPackage> packages;

    for( const std::string& directory : DirectoryLister().SetIncludeDirectories()
                                                         .SetIncludeFiles(false)
                                                         .GetPaths(m_rootDirectory) )
    {
        ASSERT(PortableFunctions::FileIsDirectory(directory));

        std::string package_spec_file_path = Path::Combine(directory, "package.json");

        if( !PortableFunctions::FileIsRegular(package_spec_file_path) )
        {
            // Get legacy csds package files too
            package_spec_file_path = PortableFunctions::PathReplaceFileExtension(package_spec_file_path, FileExtensions::DeploySpec);

            if( !PortableFunctions::FileIsRegular(package_spec_file_path) )
                continue;
        }

        std::optional<ApplicationPackage> package = GetInstalledPackageFromSpecFile(package_spec_file_path);

        if( package.has_value() )
            packages.emplace_back(std::move(*package));
    }

    return packages;
}


std::string ApplicationPackageManager::GetPackageZipFilePath(const std::string& package_name) const
{
    const std::string package_download_directory = GetPackageDownloadDirectory();

    if( !PortableFunctions::FileExists(package_download_directory) )
        PortableFunctions::PathMakeDirectory(package_download_directory);

    return PortableFunctions::CreateFilePath(package_download_directory, ReplaceInvalidFileChars(package_name, '_'), FileExtensions::Zip);
}


void ApplicationPackageManager::InstallApplication(const std::string& package_name, const std::string& downloaded_package_zip_file_path, const ApplicationPackage* const current_package) const
{
    try
    {
        std::string package_directory = ( current_package != nullptr ) ? current_package->GetInstallPath() : std::string();

        if( package_directory.empty() )
            package_directory = GetPackageInstallDirectory(package_name);

        SYNCLOG_INFO << "Installing package " << package_name << " to " << package_directory;

        const std::string package_zip_file_path = GetPackageZipFilePath(package_name);
        UpdatePackageZip(package_zip_file_path, downloaded_package_zip_file_path);

        ZipReader zip_reader(package_zip_file_path);

        // Update to an existing package - update only files with different signature
        if( current_package != nullptr )
        {
            std::optional<BinaryBlock> package_json_binary_block;

            try
            {
                package_json_binary_block = zip_reader.Read("package.json");
            }

            catch( const CSProException& )
            {
                package_json_binary_block = zip_reader.Read("package.csds");
            }

            const std::string_view new_package_json_sv = package_json_binary_block->as<std::string_view>();
            const ApplicationPackage new_package = JsonConverter::CreateApplicationPackageFromJson(Json::Parse(new_package_json_sv));

            const std::vector<ApplicationPackage::File>& current_files = current_package->GetFiles();

            for( const ApplicationPackage::File& new_file : new_package.GetFiles() )
            {
                const auto& old_file_lookup = std::find_if(current_files.begin(), current_files.end(),
                                                           [&](const ApplicationPackage::File& fs) { return ( fs.path == new_file.path ); });

                if( old_file_lookup == current_files.cend() ||
                    ( !new_file.only_on_first_install && old_file_lookup->signature != new_file.signature ) )
                {
                    std::string path_in_zip = PortableFunctions::PathToForwardSlash(new_file.path);

                    // Older (pre 7.4) package files include the .ent instead of the .pen file in the file list
                    if( SO::EqualsNoCase(PortableFunctions::PathGetFileExtension(path_in_zip), FileExtensions::EntryApplication) )
                        path_in_zip = PortableFunctions::PathReplaceFileExtension(path_in_zip, FileExtensions::BinaryEntryPen);

                    // remove the leading ./ from the path
                    if( SO::StartsWith(path_in_zip, "./") )
                        path_in_zip = path_in_zip.substr(2);

                    const std::string destination_file_path = Path::Combine(package_directory, PortableFunctions::PathToNativeSlash(path_in_zip));
                    SYNCLOG_INFO << "Updating file " << destination_file_path;

                    try
                    {
                        zip_reader.Extract(path_in_zip, destination_file_path);
                    }

                    catch( const CSProException& )
                    {
                        SYNCLOG_ERROR << "Failed to extract: " << path_in_zip << " to " << destination_file_path;
                        throw;
                    }
                }
            }

            // Update spec with one from zip - do this last so we don't change current package
            // details until all files in package are updated
            const std::string new_package_spec_file_path = Path::Combine(package_directory, "package.json");
            FileIO::WriteText(new_package_spec_file_path, new_package_json_sv, false);
        }

        // First time install - unzip everything
        else
        {
            SYNCLOG_INFO << "Unpacking all files in package";
            zip_reader.ExtractAll(package_directory);
        }

        SYNCLOG_INFO << "Installed application package to " << package_directory;
    }

    catch( const ZipException& exception )
    {
        SYNCLOG_ERROR << "Error unzipping package: " << exception.what();
        throw SyncError(100135);
    }

    catch( const JsonParseException& exception )
    {
        SYNCLOG_ERROR << "Failed to parse package JSON." << exception.what();
        throw SyncError(100150);
    }

    catch( const std::exception& exception )
    {
        SYNCLOG_ERROR << "Failed to read package JSON." << exception.what();
        throw SyncError(100150);
    }
}


void ApplicationPackageManager::UpdatePackageZip(const std::string& package_zip_file_path, const std::string& downloaded_package_zip_file_path)
{
    if( PortableFunctions::FileIsRegular(package_zip_file_path) )
    {
        ZipUtility::Combine(downloaded_package_zip_file_path, package_zip_file_path);

        PortableFunctions::FileDeleteWithExceptions(package_zip_file_path);
        PortableFunctions::FileRenameWithExceptions(downloaded_package_zip_file_path, package_zip_file_path);
    }

    else
    {
        PortableFunctions::FileCopyWithExceptions(downloaded_package_zip_file_path, package_zip_file_path, FileOverwriteFlag::Always);
    }
}
