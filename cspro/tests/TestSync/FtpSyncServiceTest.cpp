#include "stdafx.h"
#include "ApplicationsTester.h"
#include "CaseTestHelpers.h"
#include "SyncTestCredentials.h"
#include <zDataO/SyncBinaryDataUploadManager.h>
#include <zNetwork/FtpConnection.h>
#include <zSyncO/CaseObservable.h>
#include <zSyncO/FtpSyncService.h>

using namespace fakeit;


namespace SyncUnitTest
{
    TEST_CLASS(FtpSyncServiceTest)
    {
    private:
        const SyncTestCredentials::Credentials credentials = SyncTestCredentials().GetCredentialsFtp();
        const DeviceId clientDeviceId = "54ee75ad2036"; //"clientDeviceId";
        const DeviceId serverDeviceId = "54ee75ad2037"; //"serverDeviceId";

        std::unique_ptr<const CDataDict> dictionary = CreateTestDictionary();
        std::shared_ptr<CaseAccess> case_access = CaseAccess::CreateAndInitializeFullCaseAccess(*dictionary);

        std::vector<std::shared_ptr<Case>> CreateClientTestCases()
        {
            std::vector<std::shared_ptr<Case>> cases;

            VectorClock clientCaseClock;
            clientCaseClock.increment(clientDeviceId);

            for( int i = 1; i <= 2; ++i )
            {
                std::unique_ptr<Case> data_case = CreateCase(*case_access, CreateUuid(), i, { FormatText("clientdata%d", i) });
                data_case->SetVectorClock(clientCaseClock);
                cases.emplace_back(std::move(data_case));
            }

            return cases;
        }

        std::vector<std::shared_ptr<Case>> CreateServerTestCases()
        {
            std::vector<std::shared_ptr<Case>> cases;

            VectorClock serverCaseClock;
            serverCaseClock.increment(serverDeviceId);

            for( int i = 1; i <= 3; ++i )
            {
                std::unique_ptr<Case> data_case = CreateCase(*case_access, CreateUuid(), i, { FormatText("serverdata%d", i) });
                data_case->SetVectorClock(serverCaseClock);
                cases.emplace_back(std::move(data_case));
            }

            return cases;
        }

        const std::vector<std::shared_ptr<Case>> clientCases = CreateClientTestCases();
        const std::vector<std::shared_ptr<Case>> serverCases = CreateServerTestCases();

    public:
        TEST_METHOD(TestSyncDataFileGet)
        {
            Mock<FtpConnection> mockFtp;
            const std::string dictName = dictionary->GetSyncableName();
            const std::string dataParentPath = "/CSPro/DataSync/" + dictName + "/";
            const std::string dataPath = dataParentPath + "data/";

            const std::vector<std::shared_ptr<Case>> file1Cases = { serverCases[0] };
            const std::vector<std::shared_ptr<Case>> file2Cases = { serverCases[1], serverCases[2] };

            // write out the binary json in cases1Json and cases2Json
            const std::string cases1Json = GetSyncableCaseData(case_access, SpanHelpers::CreatePointersSpan(file1Cases), SyncCaseSerializer::Version::V2);
            const std::string cases2Json = GetSyncableCaseData(case_access, SpanHelpers::CreatePointersSpan(file2Cases), SyncCaseSerializer::Version::V2);

            const FileInfo dataFile1(FileInfo::FileType::File, serverDeviceId + '$' + CreateUuid(), dataPath, cases1Json.size(), 1493775345, std::string());
            const FileInfo dataFile2(FileInfo::FileType::File, serverDeviceId + '$' + CreateUuid(), dataPath, cases2Json.size(), 1493775525, std::string());

            When(Method(mockFtp, GetDirectoryListing)).Do([dataPath, this](const std::string& remote_directory_path, bool /*request_file_md5s*/) {
                // First sync returns an empty directory listing
                Assert::AreEqual(dataPath, remote_directory_path, L"URL does not match");

                std::vector<FileInfo> directory_listing;
                directory_listing.emplace_back(FileInfo::FileType::Directory, ".", dataPath);
                directory_listing.emplace_back(FileInfo::FileType::Directory, "..", dataPath);

                return directory_listing;

            }).Do([dataPath, dataFile1, dataFile2, this](const std::string& remote_directory_path, bool /*request_file_md5s*/) {

                // Second sync returns datafile1 and datafile2
                Assert::AreEqual(dataPath, remote_directory_path, L"URL does not match");

                std::vector<FileInfo> directory_listing;
                directory_listing.emplace_back(FileInfo::FileType::Directory, ".", dataPath);
                directory_listing.emplace_back(FileInfo::FileType::Directory, "..", dataPath);
                directory_listing.emplace_back(dataFile1);
                directory_listing.emplace_back(dataFile2);

                return directory_listing;

            }).Do([dataPath, dataFile1, dataFile2, this](const std::string& remote_directory_path, bool /*request_file_md5s*/) {

                // Third sync returns datafile1 and datafile2
                Assert::AreEqual(dataPath, remote_directory_path, L"URL does not match");

                std::vector<FileInfo> directory_listing;
                directory_listing.emplace_back(FileInfo::FileType::Directory, ".", dataPath);
                directory_listing.emplace_back(FileInfo::FileType::Directory, "..", dataPath);
                directory_listing.emplace_back(dataFile1);
                directory_listing.emplace_back(dataFile2);

                return directory_listing;
            });

            When(OverloadedMethod(mockFtp, Download, void(const std::string&, std::ostream&)))
                .AlwaysDo([dataFile1, dataFile2, cases1Json, cases2Json, this](const std::string& remote_file_path, std::ostream& localFileStream) {
                const std::string remoteFilename = PortableFunctions::PathGetFilename(remote_file_path);
                const size_t dollarPos = remoteFilename.find('$');
                const DeviceId deviceId = remoteFilename.substr(0, dollarPos);
                const std::string uuid = remoteFilename.substr(dollarPos + 1);
                Assert::AreEqual(serverDeviceId, deviceId);

                if (remoteFilename == dataFile1.GetName())
                    localFileStream << cases1Json;
                else if (remoteFilename == dataFile2.GetName())
                    localFileStream << cases2Json;
                else
                    Assert::Fail();
            });

            When(Method(mockFtp, FileIsRegular)).AlwaysDo([dictName, dataFile2, this](const std::string& remote_path) {
                const std::string dictPath = "/CSPro/DataSync/" + dictName + "/dict/" + dictName + FileExtensions::WithDot(FileExtensions::Dictionary);
                const std::string infoPath = "/CSPro/DataSync/" + dictName + "/dict/info.json";
                const std::string pffPath = "/CSPro/DataSync/DownloadData-" + dictName + FileExtensions::WithDot(FileExtensions::Pff);
                if (remote_path == dictPath || remote_path == infoPath || remote_path == pffPath) {
                    return true;
                } else if (PortableFunctions::PathGetFilename(remote_path) == dataFile2.GetName()) {
                    return true;
                } else {
                    Assert::Fail();
                }
            });

            When(Method(mockFtp, FileIsDirectory)).AlwaysDo([](const std::string& /*remote_path*/) {
                return true;
            });

            auto mockFtpConnectionPtr = std::shared_ptr<FtpConnection>(&mockFtp.get());
            FtpSyncService sync_service(mockFtpConnectionPtr, credentials.sync_connection_string, credentials.username_password);

            // First sync on empty directory gets nothing
            SyncGetResponse response = sync_service.GetCases(case_access, clientDeviceId, std::string(), std::string(), std::string(), std::vector<std::string>());
            Assert::AreEqual(SyncGetResponse::SyncGetResult::Complete, response.GetResult());
            Assert::AreEqual(size_t(0), ObservableToVector(*response.GetCases()).size());

            // Sync again, should get all cases
            response = sync_service.GetCases(case_access, clientDeviceId, std::string(), response.GetServerRevision(), std::string(), std::vector<std::string>());
            Assert::AreEqual(SyncGetResponse::SyncGetResult::Complete, response.GetResult());
            CompareCases(SpanHelpers::CreatePointersSpan(serverCases),
                         SpanHelpers::CreatePointersSpan(ObservableToVector(*response.GetCases())),
                         CompareCasesType::CountsAndBinaryData);

            // Sync again, should not get any new cases
            response = sync_service.GetCases(case_access, clientDeviceId, std::string(), response.GetServerRevision(), std::string(), std::vector<std::string>());
            Assert::AreEqual(SyncGetResponse::SyncGetResult::Complete, response.GetResult());
            Assert::AreEqual(size_t(0), ObservableToVector(*response.GetCases()).size());
        }


        TEST_METHOD(TestSyncDataFileGetUniverse)
        {
            Mock<FtpConnection> mockFtp;
            std::unique_ptr<const CDataDict> pDictAP = CreateTestDictionary();
            const CDataDict* pDict = pDictAP.get();
            const std::string dictName = pDict->GetSyncableName();
            const std::string dataParentPath = "/CSPro/DataSync/" + dictName + "/";
            const std::string dataPath = dataParentPath + "data/";

            const std::vector<std::shared_ptr<Case>> file1Cases = { serverCases[0] };
            const std::vector<std::shared_ptr<Case>> file2Cases = { serverCases[1], serverCases[2] };

            // write out the binary json in cases1Json and cases2Json
            const std::string& cases1Json = GetSyncableCaseData(case_access, SpanHelpers::CreatePointersSpan(file1Cases), SyncCaseSerializer::Version::V2);
            const std::string& cases2Json = GetSyncableCaseData(case_access, SpanHelpers::CreatePointersSpan(file2Cases), SyncCaseSerializer::Version::V2);

            const FileInfo dataFile1(FileInfo::FileType::File, serverDeviceId + '$' + CreateUuid(), dataPath, cases1Json.size(), 1493775345, std::string());
            const FileInfo dataFile2(FileInfo::FileType::File, serverDeviceId + '$' + CreateUuid(), dataPath, cases2Json.size(), 1493775525, std::string());


            When(Method(mockFtp, GetDirectoryListing)).Do([dataPath, dataFile1, dataFile2, this](const std::string& remote_directory_path, bool /*request_file_md5s*/) {
                // List data file directory
                Assert::AreEqual(dataPath, remote_directory_path, L"URL does not match");

                std::vector<FileInfo> directory_listing;
                directory_listing.emplace_back(FileInfo::FileType::Directory, ".", dataPath);
                directory_listing.emplace_back(FileInfo::FileType::Directory, "..", dataPath);
                directory_listing.emplace_back(dataFile1);
                directory_listing.emplace_back(dataFile2);

                return directory_listing;
            });

            When(OverloadedMethod(mockFtp, Download, void(const std::string&, std::ostream&)))
                .AlwaysDo([dataFile1, dataFile2, cases1Json, cases2Json, this, pDict](const std::string& remote_file_path, std::ostream& localFileStream) {
                const std::string remoteFilename = PortableFunctions::PathGetFilename(remote_file_path);
                const size_t dollarPos = remoteFilename.find('$');
                const DeviceId deviceId = remoteFilename.substr(0, dollarPos);
                const std::string uuid = remoteFilename.substr(dollarPos + 1);
                Assert::AreEqual(serverDeviceId, deviceId);

                if (remoteFilename == dataFile1.GetName())
                    localFileStream << cases1Json;
                else if (remoteFilename == dataFile2.GetName())
                    localFileStream << cases2Json;
                else
                    Assert::Fail();
            });

            When(Method(mockFtp, FileIsRegular)).AlwaysDo([dictName, dataFile2, this](const std::string& remote_path) {
                const std::string dictPath = "/CSPro/DataSync/" + dictName + "/dict/" + dictName + FileExtensions::WithDot(FileExtensions::Dictionary);
                const std::string infoPath = "/CSPro/DataSync/" + dictName + "/dict/info.json";
                const std::string pffPath = "/CSPro/DataSync/DownloadData-" + dictName + FileExtensions::WithDot(FileExtensions::Pff);
                if (remote_path == dictPath || remote_path == infoPath || remote_path == pffPath) {
                    return true;
                } else if (PortableFunctions::PathGetFilename(remote_path) == dataFile2.GetName()) {
                    return true;
                } else {
                    Assert::Fail();
                }
            });

            When(Method(mockFtp, FileIsDirectory)).AlwaysDo([](const std::string& /*remote_path*/) {
                return true;
            });

            auto mockFtpConnectionPtr = std::shared_ptr<FtpConnection>(&mockFtp.get());
            FtpSyncService sync_service(mockFtpConnectionPtr, credentials.sync_connection_string, credentials.username_password);

            // Sync universe of "1" should only get one case
            SyncGetResponse response = sync_service.GetCases(case_access, clientDeviceId, "  1", std::string(), std::string(), std::vector<std::string>());
            Assert::AreEqual(SyncGetResponse::SyncGetResult::Complete, response.GetResult());
            Assert::AreEqual(size_t(1), ObservableToVector(*response.GetCases()).size());
        }


        TEST_METHOD(TestApplications)
        {
            ApplicationsTester applications_tester(ApplicationPackage::DeploymentType::FTP, credentials.sync_connection_string.GetUrl());
            applications_tester.RunTest([&](SyncClient& sync_client) { return sync_client.ConnectFtp(credentials.sync_connection_string, std::make_unique<LoginCredentials>(credentials.username_password)); });
        }
    };
}
