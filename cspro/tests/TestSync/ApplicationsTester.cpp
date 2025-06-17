#include "stdafx.h"
#include "ApplicationsTester.h"
#include "CaseTestHelpers.h"
#include <zToolsO/DirectoryLister.h>
#include <zZip/ZipFile.h>
#include <zSyncO/SyncDictionaryInfo.h>
#include <zSyncO/SyncServiceFactory.h>


ApplicationsTester::ApplicationsTester(const ApplicationPackage::DeploymentType deployment_type, std::string server_url)
    :   m_deploymentType(deployment_type),
        m_serverUrl(std::move(server_url)),
        m_rootDirectory(Path::Combine(GetTempDirectory(), "ApplicationsTester-" + IntToString(GetTimestamp<int64_t>()))),
        m_packageInputsDirectory(Path::Combine(GetTempDirectory(), "ApplicationsTesterPackage-" + IntToString(GetTimestamp<int64_t>())))
{
    FileIO::CreateDirectories(m_rootDirectory);
    FileIO::CreateDirectories(m_packageInputsDirectory);

    m_applicationPackageManager = std::make_unique<ApplicationPackageManager>(m_rootDirectory);

    // the inputs for the packages will be the images shipped with CSPro
    for( const std::string& file_path : DirectoryLister(true).GetPaths(Html::GetDirectory(Html::Subdirectory::Images)) )
    {
        const std::string& input_file_path = m_inputFilePathsForPackage.emplace_back(Path::Combine(m_packageInputsDirectory, PortableFunctions::PathGetFilename(file_path)));
        PortableFunctions::FileCopyWithExceptions(file_path, input_file_path, FileOverwriteFlag::Fail);
    }
}


ApplicationsTester::~ApplicationsTester()
{
    PortableFunctions::DirectoryDelete(m_packageInputsDirectory, true);
    PortableFunctions::DirectoryDelete(m_rootDirectory, true);
}


void ApplicationsTester::RunTest(SyncClient& sync_client, const std::optional<size_t> packages_on_server_count, const std::function<void()> pre_delete_callback) const
{
    std::vector<ApplicationPackage> packages;
    SyncClient::SyncResult result;

    // test 1: make sure there are no installed packages
    packages = m_applicationPackageManager->GetInstalledApplications();
    Assert::AreEqual(size_t(0), packages.size());


    // test 2: make sure there are no packages on the server (if known)
    result = sync_client.ListApplicationPackages(packages);
    Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

    const size_t initial_packages_on_server_count = packages.size();

    if( packages_on_server_count.has_value() )
        Assert::AreEqual(*packages_on_server_count, initial_packages_on_server_count);


    // test 3: try to install a package not yet uploaded (AAA)
    const size_t number_inputs_initially_added = m_inputFilePathsForPackage.size() / 2;
    const FakePackage package_aaa = CreateFakePackage("CSPro-TestSync-ApplicationsTester-AAA-" + IntToString(GetTimestamp<int64_t>()), true, number_inputs_initially_added, 1, nullptr);
    const std::string package_aaa_install_directory = Path::Combine(m_rootDirectory, package_aaa.application_package.GetName());
    const std::string package_aaa_fake_pen_file_path = Path::Combine(package_aaa_install_directory, "fake-application.pen");

    result = sync_client.UpdateApplication(*m_applicationPackageManager, package_aaa_fake_pen_file_path);
    Assert::AreEqual(SyncClient::SyncResult::SYNC_ERROR, result);


    // test 4: upload a package (AAA)
    result = sync_client.UploadApplicationPackage(package_aaa.package_zip_temporary_file.GetPath(), package_aaa.application_package.GetName(),
                                                  package_aaa.built_application_package_json, m_packageInputsDirectory);
    Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);


    // test 5: upload another package (BBB)
    const FakePackage package_bbb = CreateFakePackage("CSPro-TestSync-ApplicationsTester-BBB-" + IntToString(GetTimestamp<int64_t>()), false, number_inputs_initially_added, 0, nullptr);
    const std::string package_bbb_install_directory = Path::Combine(m_rootDirectory, package_bbb.application_package.GetName());

    result = sync_client.UploadApplicationPackage(package_bbb.package_zip_temporary_file.GetPath(), package_bbb.application_package.GetName(),
                                                  package_bbb.built_application_package_json, m_packageInputsDirectory);
    Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);


    // test 6: check that there are now two additional packages on the server
    packages.clear();
    result = sync_client.ListApplicationPackages(packages);
    Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
    Assert::AreEqual(initial_packages_on_server_count + 2, packages.size());


    // test 7: check that the dictionary for AAA was uploaded and BBB was not
    std::vector<SyncDictionaryInfo> dictionaries;
    result = sync_client.GetDictionaries(dictionaries);
    Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
    Assert::IsTrue(std::find_if(dictionaries.cbegin(), dictionaries.cend(), [&](const SyncDictionaryInfo& sdi) { return ( sdi.GetSyncableName() == package_aaa.dictionary->GetSyncableName() ); }) != dictionaries.cend());
    Assert::IsFalse(std::find_if(dictionaries.cbegin(), dictionaries.cend(), [&](const SyncDictionaryInfo& sdi) { return ( sdi.GetSyncableName() == package_bbb.dictionary->GetSyncableName() ); }) != dictionaries.cend());


    // test 8: download packages (AAA + BBB)
    Assert::AreEqual(size_t(0), DirectoryLister(true).GetPaths(package_aaa_install_directory).size());

    result = sync_client.DownloadApplicationPackage(*m_applicationPackageManager, package_aaa.application_package.GetName(), false);
    Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

    const std::vector<std::string> package_aaa_initial_file_paths = DirectoryLister(true).GetPaths(package_aaa_install_directory);
    Assert::AreEqual(number_inputs_initially_added + 1, package_aaa_initial_file_paths.size()); // + 1 for package.json

    Assert::AreEqual(size_t(0), DirectoryLister(true).GetPaths(package_bbb_install_directory).size());

    result = sync_client.DownloadApplicationPackage(*m_applicationPackageManager, package_bbb.application_package.GetName(), true);
    Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

    const std::vector<std::string> package_bbb_initial_file_paths = DirectoryLister(true).GetPaths(package_bbb_install_directory);
    Assert::AreEqual(number_inputs_initially_added + 1, package_bbb_initial_file_paths.size());


    // test 9: redownload a package (AAA)
    result = sync_client.DownloadApplicationPackage(*m_applicationPackageManager, package_aaa.application_package.GetName(), false);
    Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
    Assert::AreEqual(package_aaa_initial_file_paths, DirectoryLister(true).GetPaths(package_aaa_install_directory));

    result = sync_client.DownloadApplicationPackage(*m_applicationPackageManager, package_aaa.application_package.GetName(), true);
    Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
    Assert::AreEqual(package_aaa_initial_file_paths, DirectoryLister(true).GetPaths(package_aaa_install_directory));


    // modify two files to test updates, only one of which should be updated
    constexpr const char* EmptyMd5 = "d41d8cd98f00b204e9800998ecf8427e";

    const std::string not_updated_source_file_path = package_aaa.application_package.GetFiles()[0].path;
    const std::string not_updated_installed_file_path = Path::Combine(package_aaa_install_directory, PortableFunctions::PathGetFilename(not_updated_source_file_path));
    const std::string not_updated_initial_md5 = PortableFunctions::FileMd5(not_updated_source_file_path);
    Assert::AreEqual(not_updated_initial_md5, PortableFunctions::FileMd5(not_updated_installed_file_path));

    FileIO::WriteText(not_updated_source_file_path, "", false);
    Assert::AreEqual(EmptyMd5, PortableFunctions::FileMd5(not_updated_source_file_path).c_str());

    const std::string updated_source_file_path = package_aaa.application_package.GetFiles()[1].path;
    const std::string updated_installed_file_path = Path::Combine(package_aaa_install_directory, PortableFunctions::PathGetFilename(updated_source_file_path));
    Assert::AreEqual(PortableFunctions::FileMd5(updated_source_file_path), PortableFunctions::FileMd5(updated_installed_file_path));

    FileIO::WriteText(updated_source_file_path, "", false);
    Assert::AreEqual(EmptyMd5, PortableFunctions::FileMd5(updated_source_file_path).c_str());


    // test 10: upload an updated package (AAA)
    const FakePackage updated_package_aaa = CreateFakePackage(package_aaa.application_package.GetName(), true, m_inputFilePathsForPackage.size(), 1, package_aaa.dictionary);
    result = sync_client.UploadApplicationPackage(updated_package_aaa.package_zip_temporary_file.GetPath(), updated_package_aaa.application_package.GetName(),
                                                  updated_package_aaa.built_application_package_json, m_packageInputsDirectory);
    Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);


    // test 11: simulate syncapp to download the updated package (AAA)
    result = sync_client.UpdateApplication(*m_applicationPackageManager, package_aaa_fake_pen_file_path);
    Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

    const std::vector<std::string> package_aaa_updated_file_paths = DirectoryLister(true).GetPaths(package_aaa_install_directory);
    Assert::AreEqual(m_inputFilePathsForPackage.size() + 1, package_aaa_updated_file_paths.size());


    // test 12: make sure that the updates came through properly
    Assert::AreEqual(not_updated_initial_md5, PortableFunctions::FileMd5(not_updated_installed_file_path));
    Assert::AreEqual(EmptyMd5, PortableFunctions::FileMd5(updated_installed_file_path).c_str());


    // test 13: make sure the installed packages are correct
    packages = m_applicationPackageManager->GetInstalledApplications();
    Assert::AreEqual(size_t(2), packages.size());

    Assert::AreEqual(updated_package_aaa.application_package.GetBuildTime(), packages[0].GetBuildTime());
    Assert::AreEqual(PortableFunctions::PathEnsureTrailingSlash(package_aaa_install_directory), PortableFunctions::PathEnsureTrailingSlash(packages[0].GetInstallPath()));

    Assert::AreEqual(package_bbb.application_package.GetBuildTime(), packages[1].GetBuildTime());
    Assert::AreEqual(PortableFunctions::PathEnsureTrailingSlash(package_bbb_install_directory), PortableFunctions::PathEnsureTrailingSlash(packages[1].GetInstallPath()));


    // test 14: delete the applications
    if( pre_delete_callback )
        pre_delete_callback();

    result = sync_client.DeleteApplication(package_aaa.application_package.GetName());
    Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

    result = sync_client.DeleteApplication(package_bbb.application_package.GetName());
    Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

    result = sync_client.DeleteApplication("nonexistent-package-name");
    Assert::AreEqual(SyncClient::SyncResult::SYNC_ERROR, result);
}


void ApplicationsTester::RunTest(const std::function<SyncClient::SyncResult(SyncClient&)>& sync_connect_callback) const
{
    ASSERT(sync_connect_callback);

    SyncClient sync_client(MakeUniqueDeviceId("TestApplications-"), std::make_unique<SyncServiceFactory>(std::make_unique<SyncLoginAccessor>()));
    sync_client.SetSyncListener(std::make_unique<SyncLogSyncListener>());

    const SyncClient::SyncResult result = sync_connect_callback(sync_client);
    Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

    RunTest(sync_client, std::nullopt, { });
}


ApplicationsTester::FakePackage ApplicationsTester::CreateFakePackage(const std::string& package_name, const bool upload_dictionary, const size_t inputs_to_include,
                                                                      const size_t inputs_to_set_only_on_first_install, std::shared_ptr<const CDataDict> dictionary) const
{
    // add files
    std::vector<ApplicationPackage::File> files;
    std::vector<ApplicationPackage::Dictionary> dictionaries;

    for( size_t i = 0; i < inputs_to_include; ++i )
    {
        const bool only_on_first_install = ( i < inputs_to_set_only_on_first_install );
        files.emplace_back(ApplicationPackage::File { m_inputFilePathsForPackage[i], std::string(), only_on_first_install });
    }

    std::vector<std::string> file_paths_for_zip;

    for( const ApplicationPackage::File& file : files )
        file_paths_for_zip.emplace_back(file.path);

    // add a dictionary
    if( dictionary == nullptr )
    {
        dictionary = CreateTestDictionaryWithUniqueName();
        const std::string dictionary_file_path = Path::Combine(m_packageInputsDirectory,
                                                               "Dicts",
                                                               PortableFunctions::PathAppendFileExtension("Dictionary-" + package_name, FileExtensions::Dictionary));
        dictionary->Save(dictionary_file_path);
    }

    dictionaries.emplace_back(ApplicationPackage::Dictionary { dictionary->GetFilePath(), upload_dictionary });

    // create the application package
    ApplicationPackage application_package(package_name,
                                           package_name + "-Description",
                                           0,
                                           m_deploymentType,
                                           m_serverUrl,
                                           std::move(files),
                                           std::move(dictionaries));

    // prepare the build, creating fake timestamps to ensure that updates will work
    static int64_t timestamp_counter = 0;
    application_package.PrepareBuild();
    application_package.SetBuildTime(GetTimestamp<int64_t>() + timestamp_counter++);

    // add package.json
    std::string built_application_package_json = application_package.GetJson(m_packageInputsDirectory, true);

    std::string application_package_json_file_path = Path::Combine(m_packageInputsDirectory, "package.json");
    FileIO::WriteText(application_package_json_file_path, built_application_package_json, false);

    file_paths_for_zip.emplace_back(std::move(application_package_json_file_path));

    // create the ZIP file
    TemporaryFile package_zip_temporary_file;
    ZipCreator(package_zip_temporary_file.GetPath()).AddFiles(file_paths_for_zip);

    return FakePackage
    {
        std::move(dictionary),
        std::move(application_package),
        std::move(built_application_package_json),
        std::move(package_zip_temporary_file)
    };
}
