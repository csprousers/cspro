#include "stdafx.h"
#include "ApplicationsTester.h"
#include <zNetwork/FileBasedConnection.h>
#include <zSyncO/SyncServiceFactory.h>


namespace SyncUnitTest
{
    TEST_CLASS(DropboxIntegrationTest)
    {
    private:
        const DeviceId clientDeviceId = "mydevice";

    public:
        TEST_METHOD(TestConnectDisconnect)
        {
            SyncClient sync_client(clientDeviceId, std::make_unique<SyncServiceFactory>());
            sync_client.SetSyncListener(std::make_unique<SyncLogSyncListener>());

            SyncClient::SyncResult result = sync_client.ConnectDropbox();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            result = sync_client.Disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
        }


        TEST_METHOD(TestPutGetFile)
        {
            SyncClient sync_client(clientDeviceId, std::make_unique<SyncServiceFactory>());
            sync_client.SetSyncListener(std::make_unique<SyncLogSyncListener>());

            SyncClient::SyncResult result = sync_client.ConnectDropbox();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // upload a file
            const std::string file_contents = CreateUuid();
            TemporaryFile upload_temporary_file;
            FileIO::WriteText(upload_temporary_file.GetPath(), file_contents, false);

            const std::string server_directory = "/CSPro-Dropbox-test/";
            const std::string server_file_path = server_directory + file_contents + ".txt";

            result = sync_client.SyncFile(SyncDirection::Put, upload_temporary_file.GetPath(), server_file_path);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // download the file
            TemporaryFile download_temporary_file;

            result = sync_client.SyncFile(SyncDirection::Get, server_file_path, download_temporary_file.GetPath());
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(Hash::Md5::Create(file_contents), Hash::Md5::CreateFromFile(download_temporary_file.GetPath()));

            // download the file again and make sure it is not re-downloaded by checking modified time
            const int64_t file_modified_time = PortableFunctions::FileModifiedTime(download_temporary_file.GetPath());

            result = sync_client.SyncFile(SyncDirection::Get, server_file_path, download_temporary_file.GetPath());
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(file_modified_time, PortableFunctions::FileModifiedTime(download_temporary_file.GetPath()), L"File was modified by second download");

            // delete the test directory on the server
            const std::shared_ptr<FileBasedConnection> file_based_connection = sync_client.GetSyncService()->GetFileBasedConnection();
            Assert::IsNotNull(file_based_connection.get());

            Assert::IsTrue(file_based_connection->FileIsDirectory(server_directory));
            file_based_connection->DirectoryDelete(server_directory);
            Assert::IsFalse(file_based_connection->FileIsDirectory(server_directory));

            // disconnect
            result = sync_client.Disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
        }


        TEST_METHOD(TestApplications)
        {
            ApplicationsTester applications_tester(ApplicationPackage::DeploymentType::Dropbox, "Dropbox");
            applications_tester.RunTest([&](SyncClient& sync_client) { return sync_client.ConnectDropbox(); });
        }
    };
}
