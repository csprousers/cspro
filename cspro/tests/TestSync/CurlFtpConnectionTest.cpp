#include "stdafx.h"
#include "SyncTestCredentials.h"
#include <zUtilO/TemporaryFile.h>
#include <zNetwork/CurlFtpConnection.h>


namespace SyncUnitTest
{
    TEST_CLASS(CurlFtpConnectionTest)
    {
    private:
        const SyncTestCredentials::Credentials credentials = SyncTestCredentials().GetCredentialsFtp();

    public:
        static TemporaryFile CreateTempFile()
        {
            TemporaryFile temp_file;
            FileIO::WriteText(temp_file.GetPath(), CreateUuid(), false);
            return temp_file;
        }


        TEST_METHOD(TestConnect)
        {
            CurlFtpConnection connection;
            connection.Connect(credentials.sync_connection_string, credentials.username_password);

            connection.Disconnect();
        }


        TEST_METHOD(TestUploadFile)
        {
            CurlFtpConnection connection;
            connection.Connect(credentials.sync_connection_string, credentials.username_password);

            const TemporaryFile upload_temp_file = CreateTempFile();

            connection.Upload(upload_temp_file.GetPath(), "/foo/bar/test-file.txt");

            connection.Disconnect();
        }


        TEST_METHOD(TestDownloadFile)
        {
            CurlFtpConnection connection;
            connection.Connect(credentials.sync_connection_string, credentials.username_password);

            const std::string remote_file_path = "/foo/bar/test-file.txt";

            const TemporaryFile upload_temp_file = CreateTempFile();
            connection.Upload(upload_temp_file.GetPath(), remote_file_path);

            const TemporaryFile download_temp_file;
            connection.Download(remote_file_path, download_temp_file.GetPath());
            Assert::AreEqual(Hash::Md5::CreateFromFile(upload_temp_file.GetPath()), Hash::Md5::CreateFromFile(download_temp_file.GetPath()));

            connection.Disconnect();
        }


        TEST_METHOD(TestDirectoryListing)
        {
            CurlFtpConnection connection;
            connection.Connect(credentials.sync_connection_string, credentials.username_password);

            // Create a temp directory on server
            const std::string server_path = "/test/" + CreateUuid() + "/";

            // Upload some files
            for( int i = 0; i < 5; ++i )
            {
                connection.Upload(CreateTempFile().GetPath(), server_path + FormatText("file%d", i));
            }

            // Create a subdirectory by uploading a file
            connection.Upload(CreateTempFile().GetPath(), server_path + "subdir/subdirfile.txt");

            const std::vector<FileInfo> directory_listing = connection.GetDirectoryListing(server_path, false);

            for( int i = 0; i < 5; ++i )
            {
                const std::string expected_name = FormatText("file%d", i);
                const auto& match = std::find_if(directory_listing.cbegin(), directory_listing.cend(),
                                                 [&](const FileInfo& i) { return i.GetName() == expected_name; });
                Assert::IsFalse(match == directory_listing.cend(), TC::ToWide(expected_name + " missing from directory listing").c_str());
                Assert::IsTrue(match->GetType() == FileInfo::FileType::File, TC::ToWide(expected_name + " not file in directory listing").c_str());
                Assert::AreEqual(int64_t(36), match->GetSize());
                Assert::AreEqual(server_path, match->GetDirectory());
            }

            const auto& match = std::find_if(directory_listing.cbegin(), directory_listing.cend(),
                                             [](const FileInfo& i) { return i.GetName() == "subdir"; });
            Assert::IsFalse(match == directory_listing.cend(), L"subdir missing from directory listing");
            Assert::IsTrue(match->GetType() == FileInfo::FileType::Directory, L"subdir not directory in directory listing");

            connection.Disconnect();
        }


        TEST_METHOD(TestFtps)
        {
            CurlFtpConnection connection;
            connection.Connect(std::string_view("ftps://test.rebex.net"), LoginCredentials("demo", "password"));

            const std::vector<FileInfo> directory_listing = connection.GetDirectoryListing("/", false);

            std::ostringstream oss;
            connection.Download("/readme.txt", oss);
            const std::string readme = oss.str();

            connection.Disconnect();
        }


        TEST_METHOD(TestFtpes)
        {
            CurlFtpConnection connection;

            connection.Connect(std::string_view("ftpes://test.rebex.net"), LoginCredentials("demo", "password"));

            const std::vector<FileInfo> directory_listing = connection.GetDirectoryListing("/", false);

            std::ostringstream oss;
            connection.Download("/readme.txt", oss);
            const std::string readme = oss.str();

            connection.Disconnect();
        }
    };
}
