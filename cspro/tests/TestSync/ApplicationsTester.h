#pragma once

#include <zUtilO/TemporaryFile.h>
#include <zSyncO/ApplicationPackageManager.h>

class ApplicationPackageManager;


class ApplicationsTester
{
public:
    ApplicationsTester(ApplicationPackage::DeploymentType deployment_type, std::string server_url);
    ~ApplicationsTester();

    void RunTest(SyncClient& sync_client, std::optional<size_t> packages_on_server_count, std::function<void()> pre_delete_callback) const;

    void RunTest(const std::function<SyncClient::SyncResult(SyncClient&)>& sync_connect_callback) const;

private:
    struct FakePackage
    {
        std::shared_ptr<const CDataDict> dictionary;
        ApplicationPackage application_package;
        std::string built_application_package_json;
        TemporaryFile package_zip_temporary_file;
    };

    FakePackage CreateFakePackage(const std::string& package_name, bool upload_dictionary, size_t inputs_to_include,
                                  size_t inputs_to_set_only_on_first_install, std::shared_ptr<const CDataDict> dictionary) const;

private:
    ApplicationPackage::DeploymentType m_deploymentType;
    std::string m_serverUrl;
    std::string m_rootDirectory;
    std::unique_ptr<ApplicationPackageManager> m_applicationPackageManager;
    std::string m_packageInputsDirectory;
    std::vector<std::string> m_inputFilePathsForPackage;
};
