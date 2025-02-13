#include "stdafx.h"
#include "CppUnitTest.h"
#include <zSyncO/SyncClient.h>
#include <zSyncO/ISyncServerConnectionFactory.h>
#include <zSyncO/ISyncServerConnection.h>
#include <zSyncO/DefaultChunk.h>
#include <zDataO/ISyncableDataRepository.h>
#include <zSyncO/SyncException.h>
#include "FakeServer.h"
#include "FakeServerFactory.h"
#include <fstream>
#include <zDictO/DDClass.h>
#include "TestRepoBuilder.h"
#include <zToolsO/Utf8Convert.h>
#include "CaseTestHelpers.h"
#include <zSyncO/CaseObservable.h>
#include <zUtilO/TemporaryFile.h>


namespace Microsoft {
    namespace VisualStudio {
        namespace CppUnitTestFramework
        {
            // ToString specialization is required for all types used in the AssertTrue macro
            template<> inline std::wstring ToString<CString>(const CString& t) { return std::wstring(t); }
            template<> inline std::wstring ToString<SyncClient::SyncResult>(const SyncClient::SyncResult& r)
            {
                switch (r) {
                case SyncClient::SyncResult::SYNC_OK:
                    return L"SyncResult::SYNC_OK";
                case SyncClient::SyncResult::SYNC_ERROR:
                    return L"SyncResult::SYNC_ERROR";
                case SyncClient::SyncResult::SYNC_CANCELED:
                    return L"SyncResult::SYNC_CANCELED";
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

    TEST_CLASS(SyncClientTest)
    {

    private:
        const DeviceId myDeviceId = "me";
        const DeviceId serverDeviceId = "myserver";
        CString hostUrl = L"www.test.com";
        CString username = L"user";
        CString password = L"pass";

        std::unique_ptr<const CDataDict> dictionary = createTestDict();
        std::shared_ptr<CaseAccess> case_access = CaseAccess::CreateAndInitializeFullCaseAccess(*dictionary);

    public:

        TEST_METHOD(TestConnect) {

            Mock<ISyncServerConnection> mockServer;
            When(Method(mockServer, connect)).Return(new ConnectResponse(serverDeviceId));
            When(Method(mockServer, disconnect)).AlwaysReturn();
            When(Method(mockServer, setListener)).AlwaysReturn();

            Mock<ISyncServerConnectionFactory> mockServerConnectionFactory;
            When(OverloadedMethod(mockServerConnectionFactory, createCSWebConnection, ISyncServerConnection*(CString, CString, CString)))
                .AlwaysReturn(&(mockServer.get()));
            When(Method(mockServerConnectionFactory, destroy)).AlwaysReturn();

            std::unique_ptr<SyncClient> pClient(new SyncClient(myDeviceId,
                &mockServerConnectionFactory.get()));

            auto connectOkResult = pClient->connectWeb(hostUrl, username, password);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, connectOkResult);
            Assert::AreEqual(serverDeviceId, pClient->getServerDeviceId());
            Verify(Method(mockServer, connect)).Exactly(1);

            auto disconnectResult = pClient->disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, disconnectResult);
            Verify(Method(mockServer, disconnect)).Exactly(1);
            Verify(Method(mockServerConnectionFactory, destroy)).Exactly(1);

            When(Method(mockServer, connect)).Throw(SyncError(1, L"connect error"));
            auto connectFailedResult = pClient->connectWeb(hostUrl, username, password);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_ERROR, connectFailedResult);
            Verify(Method(mockServer, connect)).Exactly(2);
            Verify(Method(mockServerConnectionFactory, destroy)).Exactly(2);
        }

        TEST_METHOD(TestSyncToEmptyClient) {

            // Make some cases to test with
            std::vector<std::shared_ptr<Case>> serverCases;
            VectorClock clock;
            clock.increment(serverDeviceId);
            auto serverCase1 = makeCase(*case_access, L"guid1", 1, { L"data11", L"data12" });
            serverCase1->SetVectorClock(clock);
            serverCases.push_back(serverCase1);
            auto serverCase2 = makeCase(*case_access, L"guid2", 2, { L"data2" });
            serverCase2->SetVectorClock(clock);
            serverCases.push_back(serverCase2);

            Mock<ISyncServerConnectionFactory> mockServerConnectionFactory;
            When(OverloadedMethod(mockServerConnectionFactory, createCSWebConnection, ISyncServerConnection*(CString, CString, CString)))
                .AlwaysDo([=](CString h, CString u, CString p) -> ISyncServerConnection* {return new FakeServer(serverCases);});
            When(Method(mockServerConnectionFactory, destroy)).AlwaysDo([](ISyncServerConnection* pConn)->void {delete pConn;});

            std::unique_ptr<SyncClient> pClient(new SyncClient(myDeviceId, &mockServerConnectionFactory.get()));

            TestRepoBuilder repoBuilder(myDeviceId, dictionary.get());
            ISyncableDataRepository* pRepo = repoBuilder.GetRepo();

            pClient->connectWeb(hostUrl, username, password);
            auto result = pClient->syncData(SyncDirection::Both, *pRepo, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            repoBuilder.verifyCaseInRepo(*serverCase1);
            repoBuilder.verifyCaseInRepo(*serverCase2);
        }

        TEST_METHOD(TestSyncServerMoreRecent) {

            // Make some cases to test with
            std::vector<std::shared_ptr<Case>> serverCases;
            VectorClock serverCaseClock;
            serverCaseClock.increment(serverDeviceId);
            serverCaseClock.increment(serverDeviceId);
            auto serverCase1 = makeCase(*case_access, L"guid1", 1, { "serverdata1" });
            serverCase1->SetVectorClock(serverCaseClock);
            serverCases.push_back(serverCase1);
            std::shared_ptr<Case> serverCase2 = makeCase(*case_access, L"guid2", 2, { "serverdata2" });
            serverCase2->SetVectorClock(serverCaseClock);
            serverCases.push_back(serverCase2);

            Mock<ISyncServerConnectionFactory> mockServerConnectionFactory;
            When(OverloadedMethod(mockServerConnectionFactory, createCSWebConnection, ISyncServerConnection*(CString, CString, CString)))
                .AlwaysDo([=](CString h, CString u, CString p) -> ISyncServerConnection* {return new FakeServer(serverCases);});
            When(Method(mockServerConnectionFactory, destroy)).AlwaysDo([](ISyncServerConnection* pConn)->void {delete pConn;});

            std::unique_ptr<SyncClient> pClient(new SyncClient(myDeviceId, &mockServerConnectionFactory.get()));

            TestRepoBuilder repoBuilder(myDeviceId, dictionary.get());
            ISyncableDataRepository* pRepo = repoBuilder.GetRepo();

            VectorClock clientCaseClock;
            clientCaseClock.increment(serverDeviceId);
            auto clientUpdate1 = makeCase(*case_access, L"guid1", 1, { L"clientdata1" });
            clientUpdate1->SetVectorClock(clientCaseClock);
            auto clientUpdate2 = makeCase(*case_access, L"guid2", 2, { L"clientdata2" });
            clientUpdate2->SetVectorClock(clientCaseClock);
            repoBuilder.setInitialRepoCases({ clientUpdate1, clientUpdate2}, myDeviceId);

            pClient->connectWeb(hostUrl, username, password);
            auto result = pClient->syncData(SyncDirection::Both, *pRepo, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            repoBuilder.verifyCaseInRepo(*serverCase1);
            repoBuilder.verifyCaseInRepo(*serverCase2);
        }

        TEST_METHOD(TestSyncClientMoreRecent) {

            // Make some cases to test with
            std::vector<std::shared_ptr<Case>> serverCases;
            VectorClock serverCaseClock;
            serverCaseClock.increment(serverDeviceId);
            auto serverCase1 = makeCase(*case_access, L"guid1", 1, { L"serverdata1" });
            serverCase1->SetVectorClock(serverCaseClock);
            auto serverCase2 = makeCase(*case_access, L"guid2", 1, { L"serverdata2" });
            serverCase2->SetVectorClock(serverCaseClock);
            serverCases.push_back(serverCase1);
            serverCases.push_back(serverCase2);

            Mock<ISyncServerConnectionFactory> mockServerConnectionFactory;
            When(OverloadedMethod(mockServerConnectionFactory, createCSWebConnection, ISyncServerConnection*(CString, CString, CString)))
                .AlwaysDo([=](CString h, CString u, CString p) -> ISyncServerConnection* {return new FakeServer(serverCases);});
            When(Method(mockServerConnectionFactory, destroy)).AlwaysDo([](ISyncServerConnection* pConn)->void {delete pConn;});

            std::unique_ptr<SyncClient> pClient(new SyncClient(myDeviceId, &mockServerConnectionFactory.get()));

            TestRepoBuilder repoBuilder(myDeviceId, dictionary.get());
            ISyncableDataRepository* pRepo = repoBuilder.GetRepo();

            VectorClock clientCaseClock;
            clientCaseClock.increment(serverDeviceId);
            clientCaseClock.increment(myDeviceId);
            auto clientCase1 = makeCase(*case_access, L"guid1", 1, { L"clientdata1" });
            clientCase1->SetVectorClock(clientCaseClock);
            auto clientCase2 = makeCase(*case_access, L"guid2", 2, { L"clientdata2" });
            clientCase2->SetVectorClock(clientCaseClock);
            repoBuilder.setInitialRepoCases({ clientCase1, clientCase2 }, myDeviceId);

            pClient->connectWeb(hostUrl, username, password);
            auto result = pClient->syncData(SyncDirection::Both, *pRepo, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            repoBuilder.verifyCaseInRepo(*clientCase1);
            repoBuilder.verifyCaseInRepo(*clientCase2);
        }

        TEST_METHOD(TestSyncConflictClientWins) {

            // Make some cases to test with
            std::vector<std::shared_ptr<Case>> serverCases;
            VectorClock serverCaseClock;
            serverCaseClock.increment(serverDeviceId);
            serverCaseClock.increment(serverDeviceId);
            serverCaseClock.increment(myDeviceId);
            auto serverCase1 = makeCase(*case_access, L"guid1", 1, { "serverdata1" });
            serverCase1->SetVectorClock(serverCaseClock);
            serverCases.push_back(serverCase1);
            auto serverCase2 = makeCase(*case_access, L"guid2", 2, { "serverdata2" });
            serverCase2->SetVectorClock(serverCaseClock);
            serverCases.push_back(serverCase2);

            Mock<ISyncServerConnectionFactory> mockServerConnectionFactory;
            When(OverloadedMethod(mockServerConnectionFactory, createCSWebConnection, ISyncServerConnection*(CString, CString, CString)))
                .AlwaysDo([=](CString h, CString u, CString p) -> ISyncServerConnection* {return new FakeServer(serverCases);});
            When(Method(mockServerConnectionFactory, destroy)).AlwaysDo([](ISyncServerConnection* pConn)->void {delete pConn;});

            std::unique_ptr<SyncClient> pClient(new SyncClient(myDeviceId, &mockServerConnectionFactory.get()));

            TestRepoBuilder repoBuilder(myDeviceId, dictionary.get());
            ISyncableDataRepository* pRepo = repoBuilder.GetRepo();

            VectorClock clientCaseClock;
            clientCaseClock.increment(serverDeviceId);
            clientCaseClock.increment(myDeviceId);
            clientCaseClock.increment(myDeviceId);
            auto clientCase1 = makeCase(*case_access, "guid1", 1, { "clientdata1" });
            clientCase1->SetVectorClock(clientCaseClock);
            auto clientCase2 = makeCase(*case_access, "guid2", 2, { "clientdata2" });
            clientCase2->SetVectorClock(clientCaseClock);
            std::vector<std::shared_ptr<Case>> clientCases;
            clientCases.push_back(clientCase1);
            clientCases.push_back(clientCase2);
            repoBuilder.setInitialRepoCases(clientCases, myDeviceId);

            pClient->connectWeb(hostUrl, username, password);
            auto result = pClient->syncData(SyncDirection::Both, *pRepo, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            repoBuilder.verifyCaseInRepo(*clientCase1);
            repoBuilder.verifyCaseInRepo(*clientCase2);
        }

        TEST_METHOD(TestSyncOnlyModifiedCases) {

            std::unique_ptr<const CDataDict> pDict = createTestDict();

            // Make some cases to test with
            std::vector<std::shared_ptr<Case>> serverCases;
            FakeServerFactory fakeServerFactory(serverCases);

            TestRepoBuilder repoBuilder(myDeviceId, pDict.get());
            ISyncableDataRepository* pRepo = repoBuilder.GetRepo();

            VectorClock clientCaseClock;
            clientCaseClock.increment(myDeviceId);
            auto clientCase1 = makeCase(*case_access, "guid1", 1, { "clientdata1" });
            auto clientCase2 = makeCase(*case_access, "guid2", 2, { "clientdata2" });
            std::vector<std::shared_ptr<Case>> clientCases;
            clientCases.push_back(clientCase1);
            clientCases.push_back(clientCase2);
            repoBuilder.setInitialRepoCases(clientCases, myDeviceId);

            std::unique_ptr<SyncClient> pClient(new SyncClient(myDeviceId, &fakeServerFactory));

            // First sync - should send both cases
            pClient->connectWeb(hostUrl, username, password);
            auto result = pClient->syncData(SyncDirection::Both, *pRepo, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(2, (int)fakeServerFactory.getServer()->getClientCasesFromLastRequest().size());

            // Sync again - should send nothing since no mods
            result = pClient->syncData(SyncDirection::Both, *pRepo, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(0, (int)fakeServerFactory.getServer()->getClientCasesFromLastRequest().size());

            // Modify a case and sync again - should send only modified case
            clientCaseClock.increment(myDeviceId);
            repoBuilder.updateRepoCase(clientCase1->GetUuid(), 1, L"newdata1");
            result = pClient->syncData(SyncDirection::Both, *pRepo, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(1, (int)fakeServerFactory.getServer()->getClientCasesFromLastRequest().size());
        }

        TEST_METHOD(TestSyncDataDirection) {

            // Make some cases to test with
            std::vector<std::shared_ptr<Case>> serverCases;
            VectorClock serverCaseClock;
            serverCaseClock.increment(serverDeviceId);
            auto serverCase1 = makeCase(*case_access, "guids1", 1, { "serverdata1" });
            serverCases.push_back(serverCase1);
            auto serverCase2 = makeCase(*case_access, "guids2", 2, { "serverdata2" });
            serverCases.push_back(serverCase2);

            std::unique_ptr<const CDataDict> pDict = createTestDict();

            FakeServerFactory fakeServerFactory(serverCases);

            TestRepoBuilder repoBuilder(myDeviceId, dictionary.get());
            ISyncableDataRepository* pRepo = repoBuilder.GetRepo();

            VectorClock clientCaseClock;
            clientCaseClock.increment(myDeviceId);
            auto clientCase1 = makeCase(*case_access, "guidc1", 1, { "11clientdata1" });
            clientCase1->SetVectorClock(clientCaseClock);
            auto clientCase2 = makeCase(*case_access, "guidc2", 2, { "12clientdata2" });
            clientCase2->SetVectorClock(clientCaseClock);
            std::vector<std::shared_ptr<Case>> clientCases;
            clientCases.push_back(clientCase1);
            clientCases.push_back(clientCase2);
            repoBuilder.setInitialRepoCases(clientCases, myDeviceId);

            std::unique_ptr<SyncClient> pClient(new SyncClient(myDeviceId, &fakeServerFactory));

            pClient->connectWeb(hostUrl, username, password);

            // Sync with put - should add cases to server but none to client
            auto result = pClient->syncData(SyncDirection::Put, *pRepo, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(2, (int)fakeServerFactory.getServer()->getClientCasesFromLastRequest().size());
            Assert::AreEqual(2, (int) pRepo->GetNumberCases());

            // Sync with get - should cause no change in server cases (despite new case3 added), add two cases to client
            repoBuilder.addRepoCase("guidc3", 3, "clientdata3");
            result = pClient->syncData(SyncDirection::Get, *pRepo, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(5, (int) pRepo->GetNumberCases());

            // Sync with put again - should now send only the modified case
            result = pClient->syncData(SyncDirection::Put, *pRepo, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(1, (int)fakeServerFactory.getServer()->getClientCasesFromLastRequest().size());
            Assert::AreEqual(5, (int) pRepo->GetNumberCases());

            auto disconnectResult = pClient->disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, disconnectResult);
        }

        TEST_METHOD(TestSyncDataClientLosesRevisionHistoryOnGet)
        {
            std::unique_ptr<const CDataDict> pDict = createTestDict();

            // Make some cases to test with
            std::vector<std::shared_ptr<Case>> serverCases;
            VectorClock serverCaseClock;
            serverCaseClock.increment(serverDeviceId);
            auto serverCase1 = makeCase(*case_access, "guids1", 1, { "serverdata1" });
            serverCase1->SetVectorClock(serverCaseClock);
            serverCases.push_back(serverCase1);
            auto serverCase2 = makeCase(*case_access, "guids2", 2, { "serverdata2" });
            serverCase2->SetVectorClock(serverCaseClock);
            serverCases.push_back(serverCase2);

            Mock<ISyncServerConnection> mockServer;
            DefaultDataChunk dataChunk;
            When(Method(mockServer, connect)).Return(new ConnectResponse(serverDeviceId));
            When(Method(mockServer, disconnect)).AlwaysReturn();
            When(Method(mockServer, setListener)).AlwaysReturn();
            When(Method(mockServer, getChunk)).AlwaysReturn(dataChunk);
            CString serverRev1 = "1";
            CString serverRev2 = "2";
            When(Method(mockServer, getData)).
                Do([&serverCases, serverRev1](const SyncRequest& request) {
                Assert::IsTrue(request.getLastServerRevision().IsEmpty(), L"First sync doesn't have empty server revision");
                return SyncGetResponse(SyncGetResponse::SyncGetResult::Complete, std::make_shared<CaseObservable>(rxcpp::observable<>::iterate(serverCases)), serverRev1);
            }).Do([serverRev1](const SyncRequest& request) {
                Assert::AreEqual(serverRev1, request.getLastServerRevision(), L"Second sync doesn't have revision from first sync");
                return SyncGetResponse(SyncGetResponse::SyncGetResult::RevisionNotFound);
            }).Do([&serverCases, serverRev2](const SyncRequest& request) {
                Assert::IsTrue(request.getLastServerRevision().IsEmpty(), L"Sync after revision not found doesn't have empty server revision");
                return SyncGetResponse(SyncGetResponse::SyncGetResult::Complete, std::make_shared<CaseObservable>(rxcpp::observable<>::iterate(serverCases)), serverRev2);
            });

            TestRepoBuilder repoBuilder(myDeviceId, pDict.get());
            ISyncableDataRepository* pRepo = repoBuilder.GetRepo();

            Mock<ISyncServerConnectionFactory> mockServerConnectionFactory;
            When(OverloadedMethod(mockServerConnectionFactory, createCSWebConnection, ISyncServerConnection*(CString, CString, CString)))
                .AlwaysReturn(&(mockServer.get()));
            When(Method(mockServerConnectionFactory, destroy)).AlwaysReturn();

            std::unique_ptr<SyncClient> pClient(new SyncClient(myDeviceId, &mockServerConnectionFactory.get()));

            pClient->connectWeb(hostUrl, username, password);

            // First sync, should get two cases back from server
            auto result = pClient->syncData(SyncDirection::Get, *pRepo, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            result = pClient->syncData(SyncDirection::Get, *pRepo, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            auto disconnectResult = pClient->disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, disconnectResult);

            Verify(Method(mockServer, getData)).Exactly(3);
        }

        TEST_METHOD(TestSyncDataClientLosesRevisionHistoryOnPut)
        {
            std::unique_ptr<const CDataDict> pDict = createTestDict();

            Mock<ISyncServerConnection> mockServer;
            DefaultDataChunk dataChunk;
            When(Method(mockServer, connect)).Return(new ConnectResponse(serverDeviceId));
            When(Method(mockServer, disconnect)).AlwaysReturn();
            When(Method(mockServer, setListener)).AlwaysReturn();
            When(Method(mockServer, getChunk)).AlwaysReturn(dataChunk);
            CString serverRev1 = "1";
            CString serverRev2 = "2";
            When(Method(mockServer, putData)).
                Do([serverRev1](const SyncRequest& request) {
                Assert::IsTrue(request.getLastServerRevision().IsEmpty(), L"First sync doesn't have empty server revision");
                return SyncPutResponse(SyncPutResponse::SyncPutResult::Complete, serverRev1);
            }).Do([serverRev1](const SyncRequest& request) {
                Assert::AreEqual(serverRev1, request.getLastServerRevision(), L"Second sync doesn't have revision from first sync");
                return SyncPutResponse(SyncPutResponse::SyncPutResult::RevisionNotFound);
            }).Do([serverRev2](const SyncRequest& request) {
                Assert::IsTrue(request.getLastServerRevision().IsEmpty(), L"Sync after revision not found doesn't have empty server revision");
                return SyncPutResponse(SyncPutResponse::SyncPutResult::Complete, serverRev2);
            });

            TestRepoBuilder repoBuilder(myDeviceId, pDict.get());
            ISyncableDataRepository* pRepo = repoBuilder.GetRepo();

            Mock<ISyncServerConnectionFactory> mockServerConnectionFactory;
            When(OverloadedMethod(mockServerConnectionFactory, createCSWebConnection, ISyncServerConnection*(CString, CString, CString)))
                .AlwaysReturn(&(mockServer.get()));
            When(Method(mockServerConnectionFactory, destroy)).AlwaysReturn();

            std::unique_ptr<SyncClient> pClient(new SyncClient(myDeviceId, &mockServerConnectionFactory.get()));

            pClient->connectWeb(hostUrl, username, password);

            // First sync, should get two cases back from server
            auto result = pClient->syncData(SyncDirection::Put, *pRepo, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            result = pClient->syncData(SyncDirection::Put, *pRepo, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            auto disconnectResult = pClient->disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, disconnectResult);

            Verify(Method(mockServer, putData)).Exactly(3);
        }

        TEST_METHOD(TestSyncDataChunkedGet)
        {
            std::unique_ptr<const CDataDict> pDict = createTestDict();

            // Make some cases to test with
            std::vector<std::shared_ptr<Case>> serverCases;
            VectorClock serverCaseClock;
            serverCaseClock.increment(serverDeviceId);
            for (int i = 0; i < 30; ++i) {
                CString id;
                id.Format(_T("guid%02d"), i);
                auto serverCase = makeCase(*case_access, id, 1, { "data" });
                serverCase->SetVectorClock(serverCaseClock);
                serverCases.push_back(serverCase);
            }

            Mock<ISyncServerConnection> mockServer;
            DefaultDataChunk dataChunk;
            When(Method(mockServer, connect)).Return(new ConnectResponse(serverDeviceId));
            When(Method(mockServer, disconnect)).AlwaysReturn();
            When(Method(mockServer, setListener)).AlwaysReturn();
            When(Method(mockServer, getChunk)).AlwaysReturn(dataChunk);
            CString serverRev1 = "1";
            CString serverRev2 = "2";
            CString serverRev3 = "3";
            When(Method(mockServer, getData)).
                Do([&serverCases, serverRev1](const SyncRequest& request) -> SyncGetResponse {
                    // First sync, first chunk fails
                    Assert::IsTrue(request.getLastServerRevision().IsEmpty(), L"First sync doesn't have empty server revision");
                    Assert::IsTrue(request.getLastCaseUuid().IsEmpty(), L"First sync doesn't have empty last uuid");
                    throw SyncError(100111, "some network error");
                }).Do([&serverCases, serverRev1](const SyncRequest& request) -> SyncGetResponse {
                    // Second sync, first chunk
                    std::vector<std::shared_ptr<Case>> first10;
                    std::copy(serverCases.begin(), serverCases.begin() + 10, std::back_inserter(first10));
                    Assert::IsTrue(request.getLastServerRevision().IsEmpty(), L"First sync doesn't have empty server revision");
                    Assert::IsTrue(request.getLastCaseUuid().IsEmpty(), L"First sync doesn't have empty last uuid");
                    return SyncGetResponse(SyncGetResponse::SyncGetResult::MoreData, std::make_shared<CaseObservable>(rxcpp::observable<>::iterate(first10)), serverRev1);
                }).Do([&serverCases, serverRev1](const SyncRequest& request) -> SyncGetResponse {
                    // Second sync, second chunk
                    Assert::AreEqual(serverRev1, request.getLastServerRevision());
                    Assert::AreEqual(serverCases[9]->GetUuid(), request.getLastCaseUuid());
                    throw SyncError(100111, "some network error");
                }).Do([&serverCases, serverRev1, serverRev2](const SyncRequest& request) -> SyncGetResponse {
                    // Third sync, second chunk
                    Assert::AreEqual(serverRev1, request.getLastServerRevision());
                    Assert::AreEqual(serverCases[9]->GetUuid(), request.getLastCaseUuid());
                    std::vector<std::shared_ptr<Case>> second10;
                    std::copy(serverCases.begin() + 10, serverCases.begin() + 20, std::back_inserter(second10));
                    return SyncGetResponse(SyncGetResponse::SyncGetResult::MoreData, std::make_shared<CaseObservable>(rxcpp::observable<>::iterate(second10)), serverRev2);
                }).Do([&serverCases, serverRev2, serverRev3](const SyncRequest& request) -> SyncGetResponse {
                    // Third sync, third chunk
                    Assert::AreEqual(serverRev2, request.getLastServerRevision());
                    Assert::AreEqual(serverCases[19]->GetUuid(), request.getLastCaseUuid());
                    throw SyncError(100111, "some network error");
                }).Do([&serverCases, serverRev2, serverRev3](const SyncRequest& request) -> SyncGetResponse {
                    // Fourth sync, third chunk
                    Assert::AreEqual(serverRev2, request.getLastServerRevision());
                    Assert::AreEqual(serverCases[19]->GetUuid(), request.getLastCaseUuid());
                    std::vector<std::shared_ptr<Case>> last10;
                    std::copy(serverCases.begin() + 20, serverCases.end(), std::back_inserter(last10));
                    return SyncGetResponse(SyncGetResponse::SyncGetResult::Complete, std::make_shared<CaseObservable>(rxcpp::observable<>::iterate(last10)), serverRev3);
                });

            TestRepoBuilder repoBuilder(myDeviceId, pDict.get());
            ISyncableDataRepository* pRepo = repoBuilder.GetRepo();

            Mock<ISyncServerConnectionFactory> mockServerConnectionFactory;
            When(OverloadedMethod(mockServerConnectionFactory, createCSWebConnection, ISyncServerConnection*(CString, CString, CString)))
                .AlwaysReturn(&(mockServer.get()));
            When(Method(mockServerConnectionFactory, destroy)).AlwaysReturn();

            std::unique_ptr<SyncClient> pClient(new SyncClient(myDeviceId, &mockServerConnectionFactory.get()));

            pClient->connectWeb(hostUrl, username, password);

            // First sync gets nothing
            auto result = pClient->syncData(SyncDirection::Get, *pRepo, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_ERROR, result);
            Assert::AreEqual(0, (int) pRepo->GetNumberCases());

            // Second sync should get 10 cases back from server with error
            result = pClient->syncData(SyncDirection::Get, *pRepo, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_ERROR, result);
            Assert::AreEqual(10, (int) pRepo->GetNumberCases());
            for (int i = 0; i < 10; ++i)
                repoBuilder.verifyCaseInRepo(*serverCases[i]);

            // Third should get 10 more cases back from server with error
            result = pClient->syncData(SyncDirection::Get, *pRepo, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_ERROR, result);
            Assert::AreEqual(20, (int) pRepo->GetNumberCases());
            for (int i = 0; i < 20; ++i)
                repoBuilder.verifyCaseInRepo(*serverCases[i]);

            // Fourth sync should get 10 more cases back from server with no error
            result = pClient->syncData(SyncDirection::Get, *pRepo, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(30, (int) pRepo->GetNumberCases());
            for (size_t i = 0; i < serverCases.size(); ++i)
                repoBuilder.verifyCaseInRepo(*serverCases[i]);

            auto disconnectResult = pClient->disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, disconnectResult);

            Verify(Method(mockServer, getData)).Exactly(6);
        }

        TEST_METHOD(TestSyncDataChunkedPut)
        {
            std::unique_ptr<const CDataDict> pDict = createTestDict();

            const int numCases = 25;
            const int lastChunkSize = 5;

            // Make some cases to test with
            std::vector<std::shared_ptr<Case>> clientCases;
            VectorClock clock;
            clock.increment(myDeviceId);
            for (int i = 0; i < numCases; ++i) {
                CString id;
                id.Format(_T("guid%02d"), i);
                auto clientCase = makeCase(*case_access, id, i, { "data" });
                clientCase->SetVectorClock(clock);
                clientCases.push_back(clientCase);
            }

            Mock<ISyncServerConnection> mockServer;
            DefaultDataChunk dataChunk;
            dataChunk.setSize(10);
            const int chunkSize = dataChunk.getSize();
            When(Method(mockServer, connect)).Return(new ConnectResponse(serverDeviceId));
            When(Method(mockServer, disconnect)).AlwaysReturn();
            When(Method(mockServer, setListener)).AlwaysReturn();
            When(Method(mockServer, getChunk)).AlwaysReturn(dataChunk);
            CString serverRev1 = "1";
            CString serverRev2 = "2";
            CString serverRev3 = "3";
            When(Method(mockServer, putData)).
                Do([&clientCases, chunkSize, this](const SyncRequest& request) -> SyncPutResponse {
                // First sync first chunk
                Assert::IsTrue(request.getLastServerRevision().IsEmpty(), L"First sync doesn't have empty server revision");
                Assert::AreEqual(chunkSize, (int)request.getClientCases().size());
                Assert::AreEqual(clientCases[0]->GetUuid(), request.getClientCases().front()->GetUuid());
                throw SyncError(100111, "some network error");
            }).Do([&clientCases, this, serverRev1, chunkSize](const SyncRequest& request) -> SyncPutResponse {
                // Second sync first chunk retry
                Assert::AreEqual(chunkSize, (int)request.getClientCases().size());
                Assert::AreEqual(clientCases[0]->GetUuid(), request.getClientCases().front()->GetUuid());
                Assert::IsTrue(request.getLastServerRevision().IsEmpty(), L"First sync doesn't have empty server revision");
                return SyncPutResponse(SyncPutResponse::SyncPutResult::Complete, serverRev1);
            }).Do([&clientCases, this, serverRev1, chunkSize](const SyncRequest& request) -> SyncPutResponse {
                // Second sync second chunk
                Assert::AreEqual(serverRev1, request.getLastServerRevision());
                Assert::AreEqual(chunkSize, (int)request.getClientCases().size());
                Assert::AreEqual(clientCases[chunkSize]->GetUuid(), request.getClientCases().front()->GetUuid());
                throw SyncError(100111, "some network error");
            }).Do([&clientCases, this, serverRev1, serverRev2, chunkSize](const SyncRequest& request) -> SyncPutResponse {
                // Third sync second chunk retry
                Assert::AreEqual(serverRev1, request.getLastServerRevision());
                Assert::AreEqual(chunkSize, (int)request.getClientCases().size());
                Assert::AreEqual(clientCases[chunkSize]->GetUuid(), request.getClientCases().front()->GetUuid());
                return SyncPutResponse(SyncPutResponse::SyncPutResult::Complete, serverRev2);
            }).Do([&clientCases, this, serverRev2, serverRev3, chunkSize, lastChunkSize](const SyncRequest& request) -> SyncPutResponse {
                // Third sync third chunk
                Assert::AreEqual(serverRev2, request.getLastServerRevision());
                Assert::AreEqual(lastChunkSize, (int)request.getClientCases().size());
                Assert::AreEqual(clientCases[2 * chunkSize]->GetUuid(), request.getClientCases().front()->GetUuid());
                return SyncPutResponse(SyncPutResponse::SyncPutResult::Complete, serverRev3);
            }).Do([&clientCases, this, serverRev3](const SyncRequest& request) -> SyncPutResponse {
                // Fourth sync
                Assert::AreEqual(serverRev3, request.getLastServerRevision());
                Assert::AreEqual(0, (int) request.getClientCases().size());
                return SyncPutResponse(SyncPutResponse::SyncPutResult::Complete, serverRev3);
            });

            TestRepoBuilder repoBuilder(myDeviceId, pDict.get());
            ISyncableDataRepository* pRepo = repoBuilder.GetRepo();
            repoBuilder.setInitialRepoCases(clientCases, myDeviceId);

            Mock<ISyncServerConnectionFactory> mockServerConnectionFactory;
            When(OverloadedMethod(mockServerConnectionFactory, createCSWebConnection, ISyncServerConnection*(CString, CString, CString)))
                .AlwaysReturn(&(mockServer.get()));
            When(Method(mockServerConnectionFactory, destroy)).AlwaysReturn();

            std::unique_ptr<SyncClient> pClient(new SyncClient(myDeviceId, &mockServerConnectionFactory.get()));

            pClient->connectWeb(hostUrl, username, password);

            // First sync uploads nothing - gets error
            auto result = pClient->syncData(SyncDirection::Put, *pRepo, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_ERROR, result);

            // Second sync should put 10 cases with error
            result = pClient->syncData(SyncDirection::Put, *pRepo, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_ERROR, result);

            // Third sync should put the remaining 15 cases
            result = pClient->syncData(SyncDirection::Put, *pRepo, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Fourth sync should upload nothing since everything was already uploaded
            result = pClient->syncData(SyncDirection::Put, *pRepo, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            auto disconnectResult = pClient->disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, disconnectResult);

            Verify(Method(mockServer, putData)).Exactly(6);
        }

        TEST_METHOD(TestSyncDataDontUploadDownloadedCases)
        {
            std::unique_ptr<const CDataDict> pDict = createTestDict();

            // Make some cases to test with
            std::vector<std::shared_ptr<Case>> serverCases;
            VectorClock serverCaseClock;
            serverCaseClock.increment(serverDeviceId);
            for (int i = 0; i < 10; ++i) {
                auto serverCase = makeCase(*case_access, makeUuid(), 1, { "data" });
                serverCase->SetVectorClock(serverCaseClock);
                serverCases.push_back(serverCase);
            }

            std::vector<std::shared_ptr<Case>> updatedServerCases;

            Mock<ISyncServerConnection> mockServer;
            DefaultDataChunk dataChunk;
            When(Method(mockServer, connect)).Return(new ConnectResponse(serverDeviceId));
            When(Method(mockServer, disconnect)).AlwaysReturn();
            When(Method(mockServer, setListener)).AlwaysReturn();
            When(Method(mockServer, getChunk)).AlwaysReturn(dataChunk);
            int serverRev = 1;
            When(Method(mockServer, getData)).
            Do([&serverCases, &serverRev](const SyncRequest& ) -> SyncGetResponse {
                return SyncGetResponse(SyncGetResponse::SyncGetResult::Complete, std::make_shared<CaseObservable>(rxcpp::observable<>::iterate(serverCases)), IntToString(serverRev));
            }).Do([&updatedServerCases, &serverRev](const SyncRequest&) -> SyncGetResponse {
                return SyncGetResponse(SyncGetResponse::SyncGetResult::Complete, std::make_shared<CaseObservable>(rxcpp::observable<>::iterate(updatedServerCases)), IntToString(serverRev));
            }).Do([&serverRev](const SyncRequest&) -> SyncGetResponse {
                return SyncGetResponse(SyncGetResponse::SyncGetResult::Complete, std::make_shared<CaseObservable>(rxcpp::observable<>::iterate(std::vector<std::shared_ptr<Case>>())), IntToString(serverRev));
            }).Do([](const SyncRequest& ) -> SyncGetResponse {
                return SyncGetResponse(SyncGetResponse::SyncGetResult::RevisionNotFound);
            }).Do([&serverRev](const SyncRequest&) -> SyncGetResponse {
                return SyncGetResponse(SyncGetResponse::SyncGetResult::Complete, std::make_shared<CaseObservable>(rxcpp::observable<>::iterate(std::vector<std::shared_ptr<Case>>())), IntToString(serverRev));
            });

            When(Method(mockServer, putData)).
                Do([&serverRev, &pDict](const SyncRequest& request) -> SyncPutResponse {
                // First sync should put no cases
                Assert::AreEqual(0, (int) request.getClientCases().size());
                return SyncPutResponse(SyncPutResponse::SyncPutResult::Complete, IntToString(++serverRev));
            }).Do([&serverRev, &pDict](const SyncRequest& request) -> SyncPutResponse {
                // Second sync should put 2 cases that were added/modified on client
                Assert::AreEqual(2, (int)request.getClientCases().size());
                return SyncPutResponse(SyncPutResponse::SyncPutResult::Complete, IntToString(++serverRev));
            }).Do([&serverRev, &pDict](const SyncRequest& request) -> SyncPutResponse {
                // Third sync should put nothing
                Assert::AreEqual(0, (int)request.getClientCases().size());
                return SyncPutResponse(SyncPutResponse::SyncPutResult::Complete, IntToString(++serverRev));
            }).Do([](const SyncRequest& ) -> SyncPutResponse {
                return SyncPutResponse(SyncPutResponse::SyncPutResult::RevisionNotFound);
            }).Do([&serverRev, &pDict](const SyncRequest& request) -> SyncPutResponse {
                // Third sync should upload everything
                Assert::AreEqual(12, (int)request.getClientCases().size());
                return SyncPutResponse(SyncPutResponse::SyncPutResult::Complete, IntToString(++serverRev));
            });

            TestRepoBuilder repoBuilder(myDeviceId, pDict.get());
            ISyncableDataRepository* pRepo = repoBuilder.GetRepo();

            Mock<ISyncServerConnectionFactory> mockServerConnectionFactory;
            When(OverloadedMethod(mockServerConnectionFactory, createCSWebConnection, ISyncServerConnection*(CString, CString, CString)))
                .AlwaysReturn(&(mockServer.get()));
            When(Method(mockServerConnectionFactory, destroy)).AlwaysReturn();

            std::unique_ptr<SyncClient> pClient(new SyncClient(myDeviceId, &mockServerConnectionFactory.get()));

            pClient->connectWeb(hostUrl, username, password);

            // First sync gets initial server cases, sends nothing to server
            auto result = pClient->syncData(SyncDirection::Both, *pRepo, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(10, (int) pRepo->GetNumberCases());

            // Modify/add case on server
            auto updatedServerCase1 = makeCase(*case_access, makeUuid(), 1, { "servernew" });
            updatedServerCase1->SetVectorClock(serverCaseClock);
            updatedServerCases.push_back(updatedServerCase1);
            serverCaseClock.increment(serverDeviceId);
            auto updatedServerCase2 = makeCase(*case_access, serverCases[1]->GetUuid(), 1, { "servermod" });
            updatedServerCase2->SetVectorClock(serverCaseClock);
            updatedServerCases.push_back(updatedServerCase2);

            // Modify/add case on client
            repoBuilder.addRepoCase(makeUuid(), 1, { "addedclient" });
            repoBuilder.updateRepoCase(serverCases[0]->GetUuid(), 1, { "data" });

            // Sync again, only 2 changed cases on server downloaded, only 2 changed on client uploaded
            result = pClient->syncData(SyncDirection::Both, *pRepo, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(12, (int) pRepo->GetNumberCases());

            // Next sync should put/get nothing
            result = pClient->syncData(SyncDirection::Both, *pRepo, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(12, (int) pRepo->GetNumberCases());

            // Simulate a server reset and make sure that all cases are uploaded on next sync
            serverRev = 1;
            result = pClient->syncData(SyncDirection::Both, *pRepo, L"");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(12, (int) pRepo->GetNumberCases());

            auto disconnectResult = pClient->disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, disconnectResult);
        }

        TEST_METHOD(TestSyncGetFile) {

            const std::string fileContents("SOME DATA");
            Mock<ISyncServerConnection> mockServer;
            When(Method(mockServer, connect)).Return(new ConnectResponse(serverDeviceId));
            When(Method(mockServer, disconnect)).AlwaysReturn();
            When(Method(mockServer, setListener)).AlwaysReturn();
            When(Method(mockServer, getFile)).AlwaysDo([fileContents](CString rp, CString lp, CString, CString etag = CString()) -> bool {std::ofstream os(UTF8Convert::WideToUTF8(lp), std::ios::binary);os.write(fileContents.c_str(), fileContents.size()); return true;});

            Mock<ISyncServerConnectionFactory> mockServerConnectionFactory;
            When(OverloadedMethod(mockServerConnectionFactory, createCSWebConnection, ISyncServerConnection*(CString, CString, CString)))
                .AlwaysReturn(&(mockServer.get()));
            When(Method(mockServerConnectionFactory, destroy)).AlwaysReturn();

            wchar_t pszTempPath[MAX_PATH];
            GetTempPath(MAX_PATH, pszTempPath);

            std::unique_ptr<SyncClient> pClient(new SyncClient(myDeviceId,
                &mockServerConnectionFactory.get()));

            auto connectOkResult = pClient->connectWeb(hostUrl, username, password);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, connectOkResult);

            // Test absolute path with filename
            CString destFilePath;
            destFilePath.Format(L"%szsynco-get-file-test.txt", pszTempPath);
            DeleteFile(destFilePath);
            auto syncResult = pClient->syncFile(SyncDirection::Get, L"/some/file/on/server", destFilePath, pszTempPath);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, syncResult);
            Assert::IsTrue(GetFileAttributes(destFilePath) != -1);
            std::wifstream destFile(destFilePath);
            std::string actualFileContents(std::istreambuf_iterator<wchar_t>(destFile), {});
            destFile.close();
            Assert::AreEqual(fileContents, actualFileContents);
            DeleteFile(destFilePath);

            // Absolute path with no filename - should use filename from source
            destFilePath = pszTempPath;
            CString expectedDestFilePath;
            expectedDestFilePath.Format(L"%szsynco-get-file-test.txt", pszTempPath);
            DeleteFile(expectedDestFilePath);
            syncResult = pClient->syncFile(SyncDirection::Get,
                L"/some/file/on/server/zsynco-get-file-test.txt",
                destFilePath, pszTempPath);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, syncResult);
            Assert::IsTrue(GetFileAttributes(expectedDestFilePath) != -1);
            DeleteFile(expectedDestFilePath);

            // Empty to path - should put file in file root with source name
            expectedDestFilePath.Format(L"%szsynco-get-file-test.txt", pszTempPath);
            DeleteFile(expectedDestFilePath);
            syncResult = pClient->syncFile(SyncDirection::Get,
                L"/some/path/zsynco-get-file-test.txt", CString(), pszTempPath);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, syncResult);
            Assert::IsTrue(GetFileAttributes(expectedDestFilePath) != -1);
            DeleteFile(expectedDestFilePath);

            // To path is relative - should put file relative file root
            destFilePath = "zsynco-get-file-test.txt";
            expectedDestFilePath.Format(L"%szsynco-get-file-test.txt", pszTempPath);
            DeleteFile(expectedDestFilePath);
            syncResult = pClient->syncFile(SyncDirection::Get,
                L"/some/path/somefile", destFilePath, pszTempPath);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, syncResult);
            Assert::IsTrue(GetFileAttributes(expectedDestFilePath) != -1);
            DeleteFile(expectedDestFilePath);

            // To path has parent directories that don't exist - should create them
            destFilePath = L"zsynco-get-file-test/directory/that/does/not/exist/zsynco-get-file-test.txt";
            expectedDestFilePath.Format(L"%s%s", pszTempPath, (LPCTSTR)destFilePath);
            deleteDirectoryAndContents(CString(pszTempPath) + L"zsynco-get-file-test");
            syncResult = pClient->syncFile(SyncDirection::Get,
                L"/some/path/somefile", destFilePath, pszTempPath);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, syncResult);
            Assert::IsTrue(GetFileAttributes(expectedDestFilePath) != -1);
            deleteDirectoryAndContents(CString(pszTempPath) + L"zsynco-get-file-test");

            auto disconnectResult = pClient->disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, disconnectResult);
        }

        TEST_METHOD(TestSyncGetFileError) {

            const std::string fileContents("SOME DATA");
            Mock<ISyncServerConnection> mockServer;
            When(Method(mockServer, connect)).Return(new ConnectResponse(serverDeviceId));
            When(Method(mockServer, disconnect)).AlwaysReturn();
            When(Method(mockServer, setListener)).AlwaysReturn();
            When(Method(mockServer, getFile)).AlwaysThrow(SyncError(100110, "filename"));

            Mock<ISyncServerConnectionFactory> mockServerConnectionFactory;
            When(OverloadedMethod(mockServerConnectionFactory, createCSWebConnection, ISyncServerConnection*(CString, CString, CString)))
                .AlwaysReturn(&(mockServer.get()));
            When(Method(mockServerConnectionFactory, destroy)).AlwaysReturn();

            std::unique_ptr<SyncClient> pClient(new SyncClient(myDeviceId,
                &mockServerConnectionFactory.get()));

            auto connectOkResult = pClient->connectWeb(hostUrl, username, password);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, connectOkResult);

            wchar_t pszTempPath[MAX_PATH];
            GetTempPath(MAX_PATH, pszTempPath);

            CString destFilePath;
            destFilePath.Format(L"%szsynco-get-file-test.txt", pszTempPath);
            DeleteFile(destFilePath);

            auto syncResult = pClient->syncFile(SyncDirection::Get,
                L"/some/file/on/server", destFilePath, pszTempPath);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_ERROR, syncResult);

            // File does not exist
            Assert::IsTrue(GetFileAttributes(destFilePath) == -1);

            auto disconnectResult = pClient->disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, disconnectResult);
        }

        TEST_METHOD(TestSyncGetFileWildcard) {

            const std::string fileContents("SOME DATA");
            Mock<ISyncServerConnection> mockServer;
            When(Method(mockServer, connect)).Return(new ConnectResponse(serverDeviceId));
            When(Method(mockServer, disconnect)).AlwaysReturn();
            When(Method(mockServer, setListener)).AlwaysReturn();
            When(Method(mockServer, getFile)).AlwaysDo([fileContents](CString rp, CString lp, CString, CString etag = CString()) -> bool {std::ofstream os(UTF8Convert::WideToUTF8(lp), std::ios::binary);os.write(fileContents.c_str(), fileContents.size()); return true;});
            std::vector<FileInfo> files;
            files.push_back(FileInfo(FileInfo::FileType::File, L"foo1", L"/", 10, 0, L"123124124"));
            files.push_back(FileInfo(FileInfo::FileType::File, L"foo2", L"/", 10, 0, L"123124124"));
            files.push_back(FileInfo(FileInfo::FileType::File, L"fooa", L"/", 10, 0, L"123124124"));
            files.push_back(FileInfo(FileInfo::FileType::File, L"bar", L"/", 10, 0, L"123124124"));
            files.push_back(FileInfo(FileInfo::FileType::Directory, L"foodir", L"/"));
            files.push_back(FileInfo(FileInfo::FileType::File, L"foothree", L"/", 10, 0, L"123124124"));
            When(Method(mockServer, getDirectoryListing)).AlwaysDo([files](CString dir) -> std::vector<FileInfo>* {
                return new std::vector<FileInfo>(files);
            });

            Mock<ISyncServerConnectionFactory> mockServerConnectionFactory;
            When(OverloadedMethod(mockServerConnectionFactory, createCSWebConnection, ISyncServerConnection*(CString, CString, CString)))
                .AlwaysReturn(&(mockServer.get()));
            When(Method(mockServerConnectionFactory, destroy)).AlwaysReturn();

            wchar_t pszTempPath[MAX_PATH];
            GetTempPath(MAX_PATH, pszTempPath);

            std::unique_ptr<SyncClient> pClient(new SyncClient(myDeviceId,
                &mockServerConnectionFactory.get()));

            auto connectOkResult = pClient->connectWeb(hostUrl, username, password);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, connectOkResult);

            auto syncResult = pClient->syncFile(SyncDirection::Get, L"/*", CString(), pszTempPath);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, syncResult);

            syncResult = pClient->syncFile(SyncDirection::Get, L"/foo*", CString(), pszTempPath);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, syncResult);

            syncResult = pClient->syncFile(SyncDirection::Get, L"/foo?", CString(), pszTempPath);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, syncResult);

            //# is not recognized as the wild card in the current implementation of regex. so foo1, foo2 will not be matched
            syncResult = pClient->syncFile(SyncDirection::Get, L"/foo#", CString(), pszTempPath);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, syncResult);

            Verify(Method(mockServer, getFile).Using(files[0].getDirectory() + files[0].getName(), _, _, _)).Exactly(3);
            Verify(Method(mockServer, getFile).Using(files[1].getDirectory() + files[1].getName(), _, _, _)).Exactly(3);
            Verify(Method(mockServer, getFile).Using(files[2].getDirectory() + files[2].getName(), _, _, _)).Exactly(3);
            Verify(Method(mockServer, getFile).Using(files[3].getDirectory() + files[3].getName(), _, _, _)).Once();
            Verify(Method(mockServer, getFile).Using(files[4].getDirectory() + files[4].getName(), _, _, _)).Never();
            Verify(Method(mockServer, getFile).Using(files[5].getDirectory() + files[5].getName(), _, _, _)).Twice();

            auto disconnectResult = pClient->disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, disconnectResult);
        }

        TEST_METHOD(TestSyncPutFile) {

            const std::string fileContents("SOME DATA");
            Mock<ISyncServerConnection> mockServer;
            When(Method(mockServer, connect)).Return(new ConnectResponse(serverDeviceId));
            When(Method(mockServer, disconnect)).AlwaysReturn();
            When(Method(mockServer, setListener)).AlwaysReturn();
            When(Method(mockServer, putFile)).AlwaysDo([fileContents](CString l, CString r) ->
                void {
                std::string localPathUtf8 = UTF8Convert::WideToUTF8(l);
                std::ifstream uploadStream(localPathUtf8.c_str(), std::ios::binary);
                std::string s(std::istreambuf_iterator<char>(uploadStream), {});
                Assert::AreEqual(fileContents, s);
            });

            Mock<ISyncServerConnectionFactory> mockServerConnectionFactory;
            When(OverloadedMethod(mockServerConnectionFactory, createCSWebConnection, ISyncServerConnection*(CString, CString, CString)))
                .AlwaysReturn(&(mockServer.get()));
            When(Method(mockServerConnectionFactory, destroy)).AlwaysReturn();

            wchar_t pszTempPath[MAX_PATH];
            GetTempPath(MAX_PATH, pszTempPath);

            CString srcFilename = L"zsynco-put-file-test.txt";

            CString srcFileAbsPath;
            srcFileAbsPath.Format(L"%s%s", pszTempPath, (LPCTSTR)srcFilename);
            createTestFile(srcFileAbsPath, fileContents.c_str());

            std::unique_ptr<SyncClient> pClient(new SyncClient(myDeviceId,
                &mockServerConnectionFactory.get()));

            auto connectOkResult = pClient->connectWeb(hostUrl, username, password);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, connectOkResult);

            // Test absolute dest path with filename
            CString destFilePath = "/some/folder/";
            CString fullDestFilePath = destFilePath + L"fileonserver.txt";
            auto syncResult = pClient->syncFile(SyncDirection::Put, srcFileAbsPath, fullDestFilePath, pszTempPath);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, syncResult);
            Verify(Method(mockServer, putFile).Using(_, fullDestFilePath)).Once();

            // Absolute dest path with no filename - should use filename from source
            fullDestFilePath = destFilePath + srcFilename;
            syncResult = pClient->syncFile(SyncDirection::Put, srcFileAbsPath, destFilePath, pszTempPath);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, syncResult);
            Verify(Method(mockServer, putFile).Using(_, fullDestFilePath)).Once();

            // Empty to path - should put file in file root with source name
            fullDestFilePath = _T("/") + srcFilename;
            syncResult = pClient->syncFile(SyncDirection::Put, srcFileAbsPath, CString(), pszTempPath);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, syncResult);
            Verify(Method(mockServer, putFile).Using(_, fullDestFilePath)).Once();

            // From path is relative - should find src file relative file root
            CString srcFileRelativePath = srcFilename;
            syncResult = pClient->syncFile(SyncDirection::Put, srcFileRelativePath, CString(), pszTempPath);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, syncResult);
            Verify(Method(mockServer, putFile).Using(_, fullDestFilePath)).Twice();

            auto disconnectResult = pClient->disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, disconnectResult);
        }

        TEST_METHOD(TestSyncPutFileWildcard) {

            const std::string fileContents("SOME DATA");
            Mock<ISyncServerConnection> mockServer;
            When(Method(mockServer, connect)).Return(new ConnectResponse(serverDeviceId));
            When(Method(mockServer, disconnect)).AlwaysReturn();
            When(Method(mockServer, setListener)).AlwaysReturn();
            When(Method(mockServer, putFile)).AlwaysReturn();

            Mock<ISyncServerConnectionFactory> mockServerConnectionFactory;
            When(OverloadedMethod(mockServerConnectionFactory, createCSWebConnection, ISyncServerConnection*(CString, CString, CString)))
                .AlwaysReturn(&(mockServer.get()));
            When(Method(mockServerConnectionFactory, destroy)).AlwaysReturn();

            std::unique_ptr<SyncClient> pClient(new SyncClient(myDeviceId,
                &mockServerConnectionFactory.get()));

            std::vector<CString> filenames;
            filenames.push_back(L"foo1");
            filenames.push_back(L"foo2");
            filenames.push_back(L"fooa");
            filenames.push_back(L"bar");
            filenames.push_back(L"foothree");

            wchar_t pszTempPath[MAX_PATH];
            GetTempPath(MAX_PATH, pszTempPath);
            CString srcDir;
            srcDir.Format(L"%s%s", pszTempPath, L"syncputtest\\");
            deleteDirectoryAndContents(srcDir);
            CreateDirectory(srcDir, NULL);

            for (std::vector<CString>::iterator i = filenames.begin(); i != filenames.end(); ++i) {
                CString srcFileAbsPath;
                srcFileAbsPath.Format(L"%s%s", (LPCTSTR)srcDir, (LPCTSTR)*i);
                createTestFile(srcFileAbsPath, fileContents.c_str());
            }

            CreateDirectory(srcDir + L"foodir", NULL);

            auto connectOkResult = pClient->connectWeb(hostUrl, username, password);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, connectOkResult);

            auto syncResult = pClient->syncFile(SyncDirection::Put, L"syncputtest/*", CString(), pszTempPath);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, syncResult);

            syncResult = pClient->syncFile(SyncDirection::Put, L"syncputtest/foo*", CString(), pszTempPath);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, syncResult);

            syncResult = pClient->syncFile(SyncDirection::Put, L"syncputtest/foo?", CString(), pszTempPath);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, syncResult);

            Verify(Method(mockServer, putFile).Using(_, "/" + filenames[0])).Exactly(3);
            Verify(Method(mockServer, putFile).Using(_, "/" + filenames[1])).Exactly(3);
            Verify(Method(mockServer, putFile).Using(_, "/" + filenames[2])).Exactly(3);
            Verify(Method(mockServer, putFile).Using(_, "/" + filenames[3])).Once();
            Verify(Method(mockServer, putFile).Using(_, "/" + filenames[4])).Twice();
            VerifyNoOtherInvocations(Method(mockServer, putFile));

            auto disconnectResult = pClient->disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, disconnectResult);

            deleteDirectoryAndContents(srcDir);
        }

        void createTestFile(const wchar_t* path, const char* content)
        {
            std::wofstream s(path, std::ios::binary);
            s << content;
            s.close();
        }

        int deleteDirectoryAndContents(CString dir,
            bool              bDeleteSubdirectories = true)
        {
            bool            bSubdirectory = false;       // Flag, indicating whether
                                                         // subdirectories have been found
            HANDLE          hFile;                       // Handle to directory
            CString     strFilePath;                 // Filepath
            CString     strPattern;                  // Pattern
            WIN32_FIND_DATA FileInformation;             // File information


            strPattern = dir + L"\\*.*";
            hFile = ::FindFirstFile(strPattern, &FileInformation);
            if (hFile != INVALID_HANDLE_VALUE)
            {
                do
                {
                    if (FileInformation.cFileName[0] != '.')
                    {
                        strFilePath = dir + L"\\" + FileInformation.cFileName;

                        if (FileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                        {
                            if (bDeleteSubdirectories)
                            {
                                // Delete subdirectory
                                int iRC = deleteDirectoryAndContents(strFilePath, bDeleteSubdirectories);
                                if (iRC)
                                    return iRC;
                            }
                            else
                                bSubdirectory = true;
                        }
                        else
                        {
                            // Set file attributes
                            if (::SetFileAttributes(strFilePath,
                                FILE_ATTRIBUTE_NORMAL) == FALSE)
                                return ::GetLastError();

                            // Delete file
                            if (::DeleteFile(strFilePath) == FALSE)
                                return ::GetLastError();
                        }
                    }
                } while (::FindNextFile(hFile, &FileInformation) == TRUE);

                // Close handle
                ::FindClose(hFile);

                unsigned long dwError = ::GetLastError();
                if (dwError != ERROR_NO_MORE_FILES)
                    return dwError;
                else
                {
                    if (!bSubdirectory)
                    {
                        // Set directory attributes
                        if (::SetFileAttributes(dir,
                            FILE_ATTRIBUTE_NORMAL) == FALSE)
                            return ::GetLastError();

                        // Delete directory
                        if (::RemoveDirectory(dir) == FALSE)
                            return ::GetLastError();
                    }
                }
            }

            return 0;
        }
    };
}

