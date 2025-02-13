#pragma once

#include <zSyncO/zSyncO.h>
#include <zSyncO/ApplicationPackage.h>


class SYNC_API ApplicationPackageManager {

public:

    static void installApplication(const CString &package_name,
                                   const CString &downloaded_package_zip_path,
                                   const std::optional<ApplicationPackage> &current_package);

    static std::vector<ApplicationPackage> getInstalledApplications();

    static CString getPackageZipPath(const CString& package_name);

    struct ApplicationWithSignature {
        ApplicationPackage package;
        CString signature;
    };

    static std::optional<ApplicationWithSignature> getInstalledApplicationPackageWithSignature(
        const CString &package_name);

    static std::optional<ApplicationWithSignature> getApplicationPackageWithSignatureFromAppDirectory(
            const CString &app_path);

private:

    static std::optional<std::string> getInstalledPackageJsonFromSpecFile(const CString& spec_file_name);
    static std::optional<std::string> getPackageJsonFromInstalledDirectory(const CString& installedPackageDirectory);
    static std::optional<ApplicationPackage> parsePackageSpec(const std::string& package_spec_json);
    static std::optional<ApplicationPackage> getInstalledPackageFromSpecFile(const CString& spec_file_name);
    static CString getPackageDownloadDirectory();
    static CString getPackageInstallDirectory(const CString &packageName);
    static void updatePackageZip(const CString &downloaded_zip_path,
                                 const CString &package_zip_path);
};
