#include "stdafx.h"
#include "ApplicationPackageManager.h"
#include "JsonConverter.h"
#include <zToolsO/DirectoryLister.h>
#include <zToolsO/FileIO.h>
#include <zToolsO/md5.h>
#include <zUtilO/TemporaryFile.h>
#include <zZipo/IZip.h>
#include <zSyncO/SyncException.h>
#include <easyloggingwrapper.h>


std::optional<std::string> ApplicationPackageManager::getInstalledPackageJsonFromSpecFile(const CString& spec_file_name)
{
    if (PortableFunctions::FileIsRegular(spec_file_name)) {
        try {
            return FileIO::ReadText<std::string>(spec_file_name);
        }
        catch (const std::exception& e) {
            CLOG(WARNING, "sync") << "Failed to read installed package JSON. Ignoring existing install. " << e.what();
        }
    }
    return {};
}

std::optional<std::string> ApplicationPackageManager::getPackageJsonFromInstalledDirectory(const CString& installedPackageDirectory)
{
    CString installedPackageInfoFile = PortableFunctions::PathAppendToPath(installedPackageDirectory, _T("package.json"));
    if (!PortableFunctions::FileExists(installedPackageInfoFile))
        // Older packages will have .csds instead of .json
        installedPackageInfoFile = PortableFunctions::PathRemoveFileExtension(installedPackageInfoFile) + FileExtensions::WithDot::DeploySpec;

    return getInstalledPackageJsonFromSpecFile(installedPackageInfoFile);
}

std::optional<ApplicationPackage> ApplicationPackageManager::parsePackageSpec(const std::string& package_spec_json)
{
    JsonConverter jsonConverter;

    try {
        return jsonConverter.applicationPackageFromJson(package_spec_json);
    }
    catch (const InvalidJsonException& e) {
        CLOG(WARNING, "sync") << "Failed to parse installed package JSON. Ignoring existing install. " << e.what();
    }
    return {};
}

std::optional<ApplicationPackage> ApplicationPackageManager::getInstalledPackageFromSpecFile(const CString& spec_file_name)
{
    auto package_json = getInstalledPackageJsonFromSpecFile(spec_file_name);
    if (package_json) {
        auto package = parsePackageSpec(*package_json);
        if (package)
            package->setInstallPath(PortableFunctions::PathGetDirectory(spec_file_name));
        return package;
    }
    else {
        return {};
    }
}

CString ApplicationPackageManager::getPackageInstallDirectory(const CString &packageName)
{
    return PortableFunctions::PathAppendToPath(PlatformInterface::GetInstance()->GetCSEntryDirectory(), ReplaceInvalidFileChars(packageName, _T('_')));
}

CString ApplicationPackageManager::getPackageDownloadDirectory()
{
    return PortableFunctions::PathAppendToPath(PlatformInterface::GetInstance()->GetCSEntryDirectory(), _T("Packages"));
}

std::optional<ApplicationPackageManager::ApplicationWithSignature> ApplicationPackageManager::getInstalledApplicationPackageWithSignature(
    const CString &package_name)
{
    CString installed_package_directory = getPackageInstallDirectory(package_name);
    auto installed_package_json = getPackageJsonFromInstalledDirectory(installed_package_directory);
    if (installed_package_json) {
        auto package = parsePackageSpec(*installed_package_json);
        if (package) {
            package->setInstallPath(installed_package_directory);
            return ApplicationWithSignature{*package,
                                            PortableFunctions::StringMd5(*installed_package_json)};
        }
    }

    return {};
}

std::optional<ApplicationPackageManager::ApplicationWithSignature> ApplicationPackageManager::getApplicationPackageWithSignatureFromAppDirectory(
        const CString& application_path) {
    const CString csentry_path = PlatformInterface::GetInstance()->GetCSEntryDirectory();
    int start = csentry_path.GetLength();
    if (csentry_path[csentry_path.GetLength() - 1] != _T('\\'))
        ++start;
    if (start >= application_path.GetLength())
        return {};
    int end = application_path.Find(PATH_CHAR, start);
    if (end <= start)
        return {};
    CString top_level_package_path = application_path.Left(end);
    auto installed_package_json = getPackageJsonFromInstalledDirectory(top_level_package_path);
    if (installed_package_json) {
        auto package = parsePackageSpec(*installed_package_json);
        if (package) {
            package->setInstallPath(top_level_package_path);
            return ApplicationWithSignature{*package,
                                            PortableFunctions::StringMd5(*installed_package_json)};
        }
    }

    return {};
}

std::vector<ApplicationPackage> ApplicationPackageManager::getInstalledApplications()
{
    std::vector<std::wstring> csentry_files = DirectoryLister().SetIncludeDirectories()
                                                               .GetPaths(PlatformInterface::GetInstance()->GetCSEntryDirectory());

    std::vector<CString> spec_files;
    for (const std::wstring& csentry_file : csentry_files) {
        if (PortableFunctions::FileIsDirectory(csentry_file)) {
            auto package_spec_file = PortableFunctions::PathAppendToPath(csentry_file, _T("package.json"));
            if (PortableFunctions::FileIsRegular(package_spec_file)) {
                spec_files.emplace_back(package_spec_file);
            } else {
                // Get legacy csds package files too
                package_spec_file = PortableFunctions::PathRemoveFileExtension(package_spec_file) + FileExtensions::WithDot::DeploySpec;
                if (PortableFunctions::FileIsRegular(package_spec_file)) {
                    spec_files.emplace_back(package_spec_file);
                }
            }
        }
    }
    std::vector<ApplicationPackage> apps;
    for (const auto& package_spec_file : spec_files) {
        auto package = getInstalledPackageFromSpecFile(package_spec_file);
        if (package) {
            apps.emplace_back(*package);
        }
    }
    return apps;
}

CString ApplicationPackageManager::getPackageZipPath(const CString &package_name)
{
    if (!PortableFunctions::FileExists(getPackageDownloadDirectory()))
        PortableFunctions::PathMakeDirectory(getPackageDownloadDirectory());

    return PortableFunctions::PathAppendToPath(getPackageDownloadDirectory(), ReplaceInvalidFileChars(package_name, _T('_')) + L".zip");
}

void ApplicationPackageManager::installApplication(const CString &package_name,
                                                   const CString &downloaded_package_zip_path,
                                                   const std::optional<ApplicationPackage> &current_package)
{
    try {

        CString package_directory = current_package.has_value() ? current_package->getInstallPath() : CString();
        if (package_directory.IsEmpty())
            package_directory = getPackageInstallDirectory(package_name);
        CLOG(INFO, "sync") << "Installing package " << UTF8Convert::WideToUTF8(package_name) << " to " << UTF8Convert::WideToUTF8(package_directory);

        auto package_zip_path = getPackageZipPath(package_name);
        updatePackageZip(downloaded_package_zip_path, package_zip_path);

        std::unique_ptr<IZip> pZip = std::unique_ptr<IZip>(IZip::Create());
        if (current_package) {
            // Update to an existing package - update only files with different signature
            TemporaryFile temp_package_spec;
            try {
                pZip->ExtractFile(package_zip_path, L"package.json", temp_package_spec.GetPath());
            } catch (const CZipError&) {
                pZip->ExtractFile(package_zip_path, L"package.csds", temp_package_spec.GetPath());
            }

            std::string new_package_json = FileIO::ReadText<std::string>(temp_package_spec.GetPath());
            JsonConverter jsonConverter;
            ApplicationPackage new_package = jsonConverter.applicationPackageFromJson(new_package_json);
            const auto& current_files = current_package->getFiles();
            for (const auto& new_file_spec : new_package.getFiles()) {
                auto old_file_spec = std::find_if(current_files.begin(),
                                                  current_files.end(), [new_file_spec](
                        const ApplicationPackage::FileSpec &fs) {
                        return fs.Path == new_file_spec.Path;
                    });
                if (old_file_spec == current_files.end() ||
                    (!new_file_spec.OnlyOnFirstInstall &&
                     old_file_spec->Signature != new_file_spec.Signature)) {

                    auto file_path = new_file_spec.Path;

                    // Older (pre 7.4) package files include the .ent instead of the .pen file in the file
                    // list
                    if (SO::EqualsNoCase(PortableFunctions::PathGetFileExtension(file_path), FileExtensions::EntryApplication)) {
                        file_path = PortableFunctions::PathRemoveFileExtension(file_path) + FileExtensions::WithDot::BinaryEntryPen;
                    }

                    // remove the leading ./ from the path
                    if (file_path.Left(2) == L".\\" || file_path.Left(2) == L"./")
                        file_path = file_path.Right(file_path.GetLength() - 2);
                    CString dest_path = PortableFunctions::PathAppendToPath(package_directory, PortableFunctions::PathToNativeSlash(file_path));
                    CLOG(INFO, "sync") << "Updating file "
                                       << UTF8Convert::WideToUTF8(dest_path);

                    CString path_in_zip;
                    PathCanonicalize(path_in_zip.GetBuffer(MAX_PATH), file_path);

                    // Ensure that the destination directory exists - otherwise extract fails
                    CString dest_directory = PortableFunctions::PathGetDirectory(dest_path);
                    if (!PortableFunctions::FileExists(dest_directory)) {
                        PortableFunctions::PathMakeDirectories(dest_directory);
                    }

                    try {
                        pZip->ExtractFile(downloaded_package_zip_path, path_in_zip, dest_path);
                    } catch (const CZipError& /*e*/) {
                        CLOG(ERROR, "sync") << "Failed to extract: " << path_in_zip.GetString() << " to " << dest_path.GetString();
                        throw;
                    }
                }
            }

            // Update spec with one from zip - do this last so we don't change current package
            // details until all files in package are updated
            CString new_package_spec_path = PortableFunctions::PathAppendToPath(package_directory, L"package.json");
            PortableFunctions::FileCopy(temp_package_spec.GetPath(), new_package_spec_path, false);

        } else {
            // First time install - unzip everything
            CLOG(INFO, "sync") << "Unpacking all files in package";
            pZip->ExtractAllFiles(package_zip_path, package_directory);
        }
        CLOG(INFO, "sync") << "Installed application package to " << UTF8Convert::WideToUTF8(package_directory);

    } catch (const CZipError& e) {
        CLOG(ERROR, "sync") << "Error unzipping package: " << e.m_code;
        throw SyncError(100135);
    }
    catch (const InvalidJsonException& e) {
        CLOG(ERROR, "sync") << "Failed to parse package JSON." << e.what();
        throw SyncError(100150);
    }
    catch (const std::exception& e) {
        CLOG(ERROR, "sync") << "Failed to read package JSON." << e.what();
        throw SyncError(100150);
    }
}

void ApplicationPackageManager::updatePackageZip(const CString &downloaded_zip_path,
                                                 const CString &package_zip_path)
{
    if (PortableFunctions::FileExists(package_zip_path)) {
        std::unique_ptr<IZip> pZip = std::unique_ptr<IZip>(IZip::Create());
        pZip->Merge(package_zip_path, downloaded_zip_path);
    } else {
        PortableFunctions::FileCopy(downloaded_zip_path, package_zip_path, false);
    }
}
