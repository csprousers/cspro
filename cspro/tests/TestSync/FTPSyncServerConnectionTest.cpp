#include "stdafx.h"
#include "CaseTestHelpers.h"
#include "CppUnitTest.h"
#include <zToolsO/Utf8Convert.h>
#include <zUtilO/ICredentialStore.h>
#include <zUtilO/Interapp.h>
#include <zDictO/DDClass.h>
#include <zCaseO/Case.h>
#include <zCaseO/CaseAccess.h>
#include <zNetwork/IFtpConnection.h>
#include <zSyncO/CaseObservable.h>
#include <zSyncO/ConnectResponse.h>
#include <zSyncO/FtpSyncServerConnection.h>
#include <zSyncO/ILoginDialog.h>
#include <zSyncO/JsonConverter.h>
#include <zSyncO/SyncException.h>
#include <zSyncO/SyncRequest.h>
#include <zZipo/ZipUtility.h>
#include <rxcpp/rx-lite.hpp>
#include <rxcpp/operators/rx-reduce.hpp>
#include <fstream>

namespace Microsoft {
    namespace VisualStudio {
        namespace CppUnitTestFramework
        {
            // ToString specialization is required for all types used in the AssertTrue macro
            template<> inline std::wstring ToString<CString>(const CString& t) { return std::wstring(t); }
            template<> inline std::wstring ToString<SyncGetResponse::SyncGetResult>(const SyncGetResponse::SyncGetResult& r)
            {
                switch (r) {
                case SyncGetResponse::SyncGetResult::Complete:
                    return L"SyncGetResult::Complete";
                case SyncGetResponse::SyncGetResult::MoreData:
                    return L"SyncGetResult::MoreData";
                case SyncGetResponse::SyncGetResult::RevisionNotFound:
                    return L"SyncGetResult::RevisionNotFound";
                }
                return L"THIS SHOULD NEVER HAPPEN";
            };
            template<> inline std::wstring ToString<SyncPutResponse::SyncPutResult>(const SyncPutResponse::SyncPutResult& r)
            {
                switch (r) {
                case SyncPutResponse::SyncPutResult::Complete:
                    return L"SyncPutResult::Complete";
                case SyncPutResponse::SyncPutResult::RevisionNotFound:
                    return L"SyncPutResult::RevisionNotFound";
                }
                return L"THIS SHOULD NEVER HAPPEN";
            };
        }
    }
}

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace fakeit;

namespace SyncUnitTest
{
    TEST_CLASS(FtpSyncServerConnectionTest)
    {
    private:
        CString hostUrl = "ftp://test.com/";
        CString username = "username";
        CString password = "password";
        CString clientDeviceId = "54ee75ad2036"; //"clientDeviceId";
        CString serverDeviceId = "54ee75ad2037"; //"serverDeviceId";
        JsonConverter jsonConverter;

        std::vector<std::shared_ptr<Case>> createClientTestCases()
        {
            std::vector<std::shared_ptr<Case>> cases;
            VectorClock clientCaseClock;
            clientCaseClock.increment(clientDeviceId);
            cases.emplace_back(makeCase(*case_access, makeUuid(), 1, { L"clientdata1" }));
            cases.emplace_back(makeCase(*case_access, makeUuid(), 1, { L"clientdata2" }));
            cases[0]->SetVectorClock(clientCaseClock);
            cases[1]->SetVectorClock(clientCaseClock);
            return cases;
        }

        std::vector<std::shared_ptr<Case>> createServerTestCases()
        {
            std::vector<std::shared_ptr<Case>> cases;
            VectorClock serverCaseClock;
            serverCaseClock.increment(serverDeviceId);
            cases.emplace_back(makeCase(*case_access, makeUuid(), 1, { "serverdata1" }));
            cases.emplace_back(makeCase(*case_access, makeUuid(), 2, { "serverdata2" }));
            cases.emplace_back(makeCase(*case_access, makeUuid(), 3, { "serverdata3" }));
            cases[0]->SetVectorClock(serverCaseClock);
            cases[1]->SetVectorClock(serverCaseClock);
            cases[2]->SetVectorClock(serverCaseClock);
            return cases;
        }

        std::unique_ptr<const CDataDict> dictionary = createTestDict();
        std::shared_ptr<CaseAccess> case_access = CaseAccess::CreateAndInitializeFullCaseAccess(*dictionary);
        std::vector<std::shared_ptr<Case>> serverCases = createServerTestCases();
        std::vector<std::shared_ptr<Case>> clientCases = createClientTestCases();

    public:

        TEST_METHOD(TestSyncDataFileGet)
        {
            Mock<IFtpConnection> mockFtp;
            CString dictName = dictionary->GetName();
            CString dataParentPath = _T("/CSPro/DataSync/") + dictName + _T("/");
            CString dataPath = dataParentPath + _T("data/");

            std::vector<std::shared_ptr<Case>> file1Cases = { serverCases[0] };
            std::vector<std::shared_ptr<Case>> file2Cases = { serverCases[1], serverCases[2] };

            // write out the binary json in cases1Json and cases2Json
            const std::string& cases1Json = convertCasesToJSON(jsonConverter, file1Cases);
            const std::string& cases2Json = convertCasesToJSON(jsonConverter, file2Cases);

            const FileInfo dataFile1(FileInfo::FileType::File, serverDeviceId + _T('$') + makeUuid(), dataPath, cases1Json.size(), 1493775345, std::wstring());
            const FileInfo dataFile2(FileInfo::FileType::File, serverDeviceId + _T('$') + makeUuid(), dataPath, cases2Json.size(), 1493775525, std::wstring());


            When(Method(mockFtp, connect)).Do([](CString, CString, CString) {

            });


            When(Method(mockFtp, getDirectoryListing)).Do([dataParentPath, this](CString path) {

                // Check if data dir exists
                Assert::AreEqual(dataParentPath, path);

                std::vector<FileInfo>* pFiles = new std::vector<FileInfo>();
                pFiles->push_back(FileInfo(FileInfo::FileType::Directory, L"data", dataParentPath));

                return pFiles;
            }).Do([dataPath, this](CString path) {

                // First sync returns an empty directory listing
                Assert::AreEqual(dataPath, path, L"URL does not match");

                std::vector<FileInfo>* pFiles = new std::vector<FileInfo>();
                pFiles->push_back(FileInfo(FileInfo::FileType::Directory, L".", dataPath));
                pFiles->push_back(FileInfo(FileInfo::FileType::Directory, L"..", dataPath));

                return pFiles;
            }).Do([dataParentPath, this](CString path) {

                // Check if data dir exists
                Assert::AreEqual(dataParentPath, path);

                std::vector<FileInfo>* pFiles = new std::vector<FileInfo>();
                pFiles->push_back(FileInfo(FileInfo::FileType::Directory, L"data", dataParentPath));

                return pFiles;
            }).Do([dataPath, dataFile1, dataFile2, this](CString path) {

                // Second sync returns datafile1 and datafile2
                Assert::AreEqual(dataPath, path, L"URL does not match");

                std::vector<FileInfo>* pFiles = new std::vector<FileInfo>();
                pFiles->push_back(FileInfo(FileInfo::FileType::Directory, L".", dataPath));
                pFiles->push_back(FileInfo(FileInfo::FileType::Directory, L"..", dataPath));
                pFiles->push_back(dataFile1);
                pFiles->push_back(dataFile2);

                return pFiles;
            }).Do([dataParentPath, this](CString path) {

                // Check if data dir exists
                Assert::AreEqual(dataParentPath, path);

                std::vector<FileInfo>* pFiles = new std::vector<FileInfo>();
                pFiles->push_back(FileInfo(FileInfo::FileType::Directory, L"data", dataParentPath));

                return pFiles;
            }).Do([dataPath, dataFile1, dataFile2, this](CString path) {

                // Third sync returns datafile1 and datafile2
                Assert::AreEqual(dataPath, path, L"URL does not match");

                std::vector<FileInfo>* pFiles = new std::vector<FileInfo>();
                pFiles->push_back(FileInfo(FileInfo::FileType::Directory, L".", dataPath));
                pFiles->push_back(FileInfo(FileInfo::FileType::Directory, L"..", dataPath));
                pFiles->push_back(dataFile1);
                pFiles->push_back(dataFile2);

                return pFiles;
            });

            When(OverloadedMethod(mockFtp, download, void(CString , std::ostream&)))
                .AlwaysDo([dataFile1, dataFile2, cases1Json, cases2Json, this](CString remoteFilePath, std::ostream& localFileStream) {
                CString remoteFilename = PortableFunctions::PathGetFilename(remoteFilePath);
                int dollarPos = remoteFilename.Find(L'$');
                CString deviceId = remoteFilename.Left(dollarPos);
                CString uuid = remoteFilename.Mid(dollarPos + 1);
                Assert::AreEqual(serverDeviceId, deviceId);

                if (remoteFilename == dataFile1.getName())
                    localFileStream << cases1Json;
                else if (remoteFilename == dataFile2.getName())
                    localFileStream << cases2Json;
                else
                    Assert::Fail();
            });

            When(Method(mockFtp, getLastModifiedTime)).AlwaysDo([dictName, dataFile2, this](CString remoteFilePath) {
                const CString dictPath = _T("/CSPro/DataSync/") + dictName + _T("/dict/") + dictName + FileExtensions::WithDot::Dictionary;
                const CString infoPath = _T("/CSPro/DataSync/") + dictName + _T("/dict/info.json");
                const CString pffPath = _T("/CSPro/DataSync/DownloadData-") + dictName + FileExtensions::WithDot::Pff;
                if (remoteFilePath == dictPath || remoteFilePath == infoPath || remoteFilePath == pffPath) {
                    return time_t(1); // greater than zero - exists
                } else if (PortableFunctions::PathGetFilename(remoteFilePath) == dataFile2.getName()) {
                    return dataFile2.getLastModified();
                } else {
                    Assert::Fail();
                }
                return time_t(0);
            });

            auto mockFtpConnectionPtr = std::shared_ptr<IFtpConnection>(&mockFtp.get(), [](...) {});
            FtpSyncServerConnection connection(mockFtpConnectionPtr, hostUrl, username, password);

            // First sync on empty directory gets nothing
            auto response = connection.getData(SyncRequest(*case_access, clientDeviceId, L"", CString(), CString(), std::vector<CString>()));
            Assert::AreEqual(SyncGetResponse::SyncGetResult::Complete, response.getResult());
            Assert::AreEqual(0, (int) ObservableToVector(*response.getCases()).size());

            // Sync again, should get all cases
            response = connection.getData(SyncRequest(*case_access, clientDeviceId, L"", response.getServerRevision(), CString(), std::vector<CString>()));
            Assert::AreEqual(SyncGetResponse::SyncGetResult::Complete, response.getResult());
            checkCaseList(serverCases, ObservableToVector(*response.getCases()));

            // Sync again, should not get any new cases
            response = connection.getData(SyncRequest(*case_access, clientDeviceId, L"", response.getServerRevision(), CString(), std::vector<CString>()));
            Assert::AreEqual(SyncGetResponse::SyncGetResult::Complete, response.getResult());
            Assert::AreEqual(0, (int)ObservableToVector(*response.getCases()).size());
        }

        TEST_METHOD(TestSyncDataFileGetUniverse)
        {
            Mock<IFtpConnection> mockFtp;
            std::unique_ptr<const CDataDict> pDictAP = createTestDict();
            const CDataDict* pDict = pDictAP.get();
            CString dictName = pDict->GetName();
            CString dataParentPath = _T("/CSPro/DataSync/") + dictName + _T("/");
            CString dataPath = dataParentPath + _T("data/");

            std::vector<std::shared_ptr<Case>> file1Cases = { serverCases[0] };
            std::vector<std::shared_ptr<Case>> file2Cases = { serverCases[1], serverCases[2] };
   

            // write out the binary json in cases1Json and cases2Json
            const std::string& cases1Json = convertCasesToJSON(jsonConverter, file1Cases);
            const std::string& cases2Json = convertCasesToJSON(jsonConverter, file2Cases);

            const FileInfo dataFile1(FileInfo::FileType::File, serverDeviceId + _T('$') + makeUuid(), dataPath, cases1Json.size(), 1493775345, std::wstring());
            const FileInfo dataFile2(FileInfo::FileType::File, serverDeviceId + _T('$') + makeUuid(), dataPath, cases2Json.size(), 1493775525, std::wstring());


            When(Method(mockFtp, connect)).Do([](CString, CString, CString) {

            });


            When(Method(mockFtp, getDirectoryListing)).Do([dataParentPath, this](CString path) {

                // Check if data dir exists
                Assert::AreEqual(dataParentPath, path);

                std::vector<FileInfo>* pFiles = new std::vector<FileInfo>();
                pFiles->push_back(FileInfo(FileInfo::FileType::Directory, L"data", dataParentPath));

                return pFiles;
            }).Do([dataPath, dataFile1, dataFile2, this](CString path) {

                // List data file directory
                Assert::AreEqual(dataPath, path, L"URL does not match");

                std::vector<FileInfo>* pFiles = new std::vector<FileInfo>();
                pFiles->push_back(FileInfo(FileInfo::FileType::Directory, L".", dataPath));
                pFiles->push_back(FileInfo(FileInfo::FileType::Directory, L"..", dataPath));
                pFiles->push_back(dataFile1);
                pFiles->push_back(dataFile2);

                return pFiles;
            });

            When(OverloadedMethod(mockFtp, download, void(CString, std::ostream&)))
                .AlwaysDo([dataFile1, dataFile2, cases1Json, cases2Json, this, pDict](CString remoteFilePath, std::ostream& localFileStream) {
                CString remoteFilename = PortableFunctions::PathGetFilename(remoteFilePath);
                int dollarPos = remoteFilename.Find(L'$');
                CString deviceId = remoteFilename.Left(dollarPos);
                CString uuid = remoteFilename.Mid(dollarPos + 1);
                Assert::AreEqual(serverDeviceId, deviceId);

                if (remoteFilename == dataFile1.getName())
                    localFileStream << cases1Json;
                else if (remoteFilename == dataFile2.getName())
                    localFileStream << cases2Json;
                else
                    Assert::Fail();
            });

            When(Method(mockFtp, getLastModifiedTime)).AlwaysDo([dictName, dataFile2, this](CString remoteFilePath) {
                const CString dictPath = _T("/CSPro/DataSync/") + dictName + _T("/dict/") + dictName + FileExtensions::WithDot::Dictionary;
                const CString infoPath = _T("/CSPro/DataSync/") + dictName + _T("/dict/info.json");
                const CString pffPath = _T("/CSPro/DataSync/DownloadData-") + dictName + FileExtensions::WithDot::Pff;
                if (remoteFilePath == dictPath || remoteFilePath == infoPath || remoteFilePath == pffPath) {
                    return time_t(1); // greater than zero - exists
                } else if (PortableFunctions::PathGetFilename(remoteFilePath) == dataFile2.getName()) {
                    return dataFile2.getLastModified();
                } else {
                    Assert::Fail();
                }
                return time_t(0);
            });

            auto mockFtpConnectionPtr = std::shared_ptr<IFtpConnection>(&mockFtp.get(), [](...) {});
            FtpSyncServerConnection connection(mockFtpConnectionPtr, hostUrl, username, password);

            // Sync universe of "1" should only get one case
            auto response = connection.getData(SyncRequest(*case_access, clientDeviceId, L"1", CString(), CString(), std::vector<CString>()));
            Assert::AreEqual(SyncGetResponse::SyncGetResult::Complete, response.getResult());
            Assert::AreEqual(1, (int)ObservableToVector(*response.getCases()).size());

        }

        TEST_METHOD(TestSyncDataFilePut)
        {
            Mock<IFtpConnection> mockFtp;
            std::unique_ptr<const CDataDict> pDictAP = createTestDict();
            const CDataDict* pDict = pDictAP.get();
            CString dictName = pDict->GetName();
            CString dataParentPath = _T("/CSPro/DataSync/") + dictName + _T("/");
            CString dataPath = dataParentPath + _T("data/");

            When(Method(mockFtp, connect)).Do([](CString, CString, CString) {

            });

            When(OverloadedMethod(mockFtp, upload, void(std::istream&, int64_t, CString)))
                .AlwaysDo([&dataPath, this](std::istream& localFileData, int64_t, CString remoteFilePath) {

                Assert::AreEqual(dataPath, PortableFunctions::PathGetDirectory<CString>(remoteFilePath));
                CString remoteFilename = PortableFunctions::PathGetFilename(remoteFilePath);
                int dollarPos = remoteFilename.Find(L'$');
                CString deviceId = remoteFilename.Left(dollarPos);
                CString uuid = remoteFilename.Mid(dollarPos + 1);
                Assert::AreEqual(36, uuid.GetLength());
                Assert::AreEqual(clientDeviceId, deviceId);
                std::string casesJson(std::istreambuf_iterator<char>(localFileData), {});
                ZipUtility::decompression(casesJson);
                std::vector<std::shared_ptr<Case>> cases;

                cases = getCaseListFromJson(jsonConverter, casesJson,*case_access);
                //check case list and the binary file.
                checkCaseList(clientCases, cases);
            });

            When(Method(mockFtp, getDirectoryListing)).AlwaysDo([dataParentPath, this](CString path) {

                // Check if data dir exists
                Assert::AreEqual(dataParentPath, path);

                std::vector<FileInfo>* pFiles = new std::vector<FileInfo>();
                pFiles->push_back(FileInfo(FileInfo::FileType::Directory, L"data", dataParentPath));

                return pFiles;
            });

            When(Method(mockFtp, getLastModifiedTime)).AlwaysDo([dictName, this](CString remoteFilePath) {
                return time_t(1492816560);
            });

            auto mockFtpConnectionPtr = std::shared_ptr<IFtpConnection>(&mockFtp.get(), [](...) {});
            FtpSyncServerConnection connection(mockFtpConnectionPtr, hostUrl, username, password);

            std::vector<std::pair<const BinaryCaseItem*, CaseItemIndex>> binary_case_items;

            const BinaryCaseItem* servercase_binary_case_item = dynamic_cast<const BinaryCaseItem*>(clientCases[0]->GetRootCaseLevel().GetCaseRecord(0).GetCaseItems()[1]);
            auto servercase_binary_case_item_index = clientCases[0]->GetRootCaseLevel().GetCaseRecord(0).GetCaseItemIndex(0);

            std::pair<const BinaryCaseItem*, CaseItemIndex> binary = std::make_pair(servercase_binary_case_item, servercase_binary_case_item_index);
            binary_case_items.push_back(binary);


            SyncRequest request(*case_access, clientDeviceId, L"", clientCases, binary_case_items, CString());
            auto response = connection.putData(request);
            Assert::AreEqual(SyncPutResponse::SyncPutResult::Complete, response.getResult());
        }
    };
}
