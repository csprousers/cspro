#pragma once

#include <zSyncO/zSyncO.h>
#include <zSyncO/ApplicationPackage.h>


class SYNC_API ApplicationPackageManager
{
public:
    ApplicationPackageManager(std::string root_directory);

    void InstallApplication(const std::string& package_name, const std::string& downloaded_package_zip_file_path, const ApplicationPackage* current_package) const;

    std::vector<ApplicationPackage> GetInstalledApplications() const;

    std::string GetPackageZipFilePath(const std::string& package_name) const;

    struct ApplicationWithSignature
    {
        ApplicationPackage package;
        std::string signature;
    };

    std::unique_ptr<ApplicationWithSignature> GetInstalledApplicationPackageWithSignature(const std::string& package_name) const;

    std::unique_ptr<ApplicationWithSignature> GetApplicationPackageWithSignatureFromApplicationDirectory(const std::string& application_file_path) const;

private:
    static std::optional<std::string> GetInstalledPackageJsonFromSpecFile(const std::string& package_spec_file_path);
    static std::optional<std::string> GetPackageJsonFromInstalledDirectory(const std::string& installed_package_directory);
    static std::optional<ApplicationPackage> ParsePackageSpec(const std::string& package_spec_json);
    static std::optional<ApplicationPackage> GetInstalledPackageFromSpecFile(const std::string& package_spec_file_path);
    static std::unique_ptr<ApplicationWithSignature> GetApplicationPackageWithSignatureFromInstalledDirectory(const std::string& installed_package_directory);
    std::string GetPackageInstallDirectory(const std::string& package_name) const;
    std::string GetPackageDownloadDirectory() const;
    static void UpdatePackageZip(const std::string& package_zip_file_path, const std::string& downloaded_package_zip_file_path);

private:
    std::string m_rootDirectory;
};
