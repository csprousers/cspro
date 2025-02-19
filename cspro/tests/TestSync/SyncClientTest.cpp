#include "stdafx.h"
#include "CaseTestHelpers.h"
#include "FakeSyncService.h"
#include "FakeSyncServiceFactory.h"
#include "TestRepoBuilder.h"
#include <zDictO/DDClass.h>
#include <zDataO/ISyncableDataRepository.h>
#include <zSyncO/ISyncService.h>
#include <fstream>

using namespace fakeit;


namespace SyncUnitTest
{
    TEST_CLASS(SyncClientTest)
    {
    private:
        const DeviceId myDeviceId = "me";
        const DeviceId serverDeviceId = "myserver";
        const std::string hostUrl = "http://www.test.com";
        const std::string username = "user";
        const std::string password = "pass";

        const std::shared_ptr<const CDataDict> dictionary = CreateTestDictionary();
        const std::shared_ptr<CaseAccess> case_access = CaseAccess::CreateAndInitializeFullCaseAccess(*dictionary);

    public:
        TEST_METHOD(TestConnect)
        {
            Mock<ISyncService> mockServer;
            When(Method(mockServer, Connect)).Return(std::make_shared<ConnectResponse>(serverDeviceId));
            When(Method(mockServer, Disconnect)).AlwaysReturn();
            When(Method(mockServer, SetSyncListener)).AlwaysReturn();
            When(Method(mockServer, GetSharedSyncListener)).AlwaysReturn(nullptr);

            Mock<ISyncServiceFactory> mockSyncServiceFactory;
            When(OverloadedMethod(mockSyncServiceFactory, CreateCSWebSyncService, std::unique_ptr<ISyncService>(SyncConnectionString, std::unique_ptr<LoginCredentials>)))
                .AlwaysDo([&](const SyncConnectionString&, const std::unique_ptr<LoginCredentials>&) { return std::unique_ptr<ISyncService>(&(mockServer.get())); });

            SyncClient sync_client(myDeviceId, std::unique_ptr<ISyncServiceFactory>(&mockSyncServiceFactory.get()));
            sync_client.SetSyncListener(std::make_unique<SyncLogSyncListener>());

            auto connectOkResult = sync_client.ConnectCSWeb(hostUrl, std::make_unique<LoginCredentials>(username, password));
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, connectOkResult);
            Assert::AreEqual(serverDeviceId, sync_client.GetServerDeviceId());
            Verify(Method(mockServer, Connect)).Exactly(1);

            auto disconnectResult = sync_client.Disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, disconnectResult);
            Verify(Method(mockServer, Disconnect)).Exactly(1);

            When(Method(mockServer, Connect)).Throw(SyncError(1, "connect error"));
            auto connectFailedResult = sync_client.ConnectCSWeb(hostUrl, std::make_unique<LoginCredentials>(username, password));
            Assert::AreEqual(SyncClient::SyncResult::SYNC_ERROR, connectFailedResult);
            Verify(Method(mockServer, Connect)).Exactly(2);
        }


        TEST_METHOD(TestSyncToEmptyClient)
        {
            // Make some cases to test with
            std::vector<std::shared_ptr<Case>> serverCases;
            VectorClock clock;
            clock.increment(serverDeviceId);
            std::shared_ptr<Case> serverCase1 = CreateCase(*case_access, "guid1", 1, { "data11", "data12" });
            serverCase1->SetVectorClock(clock);
            serverCases.emplace_back(serverCase1);
            std::shared_ptr<Case> serverCase2 = CreateCase(*case_access, "guid2", 2, { "data2" });
            serverCase2->SetVectorClock(clock);
            serverCases.emplace_back(serverCase2);

            Mock<ISyncServiceFactory> mockSyncServiceFactory;
            When(OverloadedMethod(mockSyncServiceFactory, CreateCSWebSyncService, std::unique_ptr<ISyncService>(SyncConnectionString, std::unique_ptr<LoginCredentials>)))
                .AlwaysDo([&](const SyncConnectionString&, const std::unique_ptr<LoginCredentials>&) { return std::make_unique<FakeSyncService>(serverCases); });

            SyncClient sync_client(myDeviceId, std::unique_ptr<ISyncServiceFactory>(&mockSyncServiceFactory.get()));
            sync_client.SetSyncListener(std::make_unique<SyncLogSyncListener>());

            TestRepoBuilder repoBuilder(myDeviceId, dictionary.get());
            ISyncableDataRepository* pRepo = repoBuilder.GetRepo();

            sync_client.ConnectCSWeb(hostUrl, std::make_unique<LoginCredentials>(username, password));
            const SyncClient::SyncResult result = sync_client.SyncData(SyncDirection::Both, *pRepo, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            repoBuilder.verifyCaseInRepo(*serverCase1);
            repoBuilder.verifyCaseInRepo(*serverCase2);
        }


        TEST_METHOD(TestSyncServiceMoreRecent)
        {
            // Make some cases to test with
            std::vector<std::shared_ptr<Case>> serverCases;
            VectorClock serverCaseClock;
            serverCaseClock.increment(serverDeviceId);
            serverCaseClock.increment(serverDeviceId);
            std::shared_ptr<Case> serverCase1 = CreateCase(*case_access, "guid1", 1, { "serverdata1" });
            serverCase1->SetVectorClock(serverCaseClock);
            serverCases.emplace_back(serverCase1);
            std::shared_ptr<Case> serverCase2 = CreateCase(*case_access, "guid2", 2, { "serverdata2" });
            serverCase2->SetVectorClock(serverCaseClock);
            serverCases.emplace_back(serverCase2);

            Mock<ISyncServiceFactory> mockSyncServiceFactory;
            When(OverloadedMethod(mockSyncServiceFactory, CreateCSWebSyncService, std::unique_ptr<ISyncService>(SyncConnectionString, std::unique_ptr<LoginCredentials>)))
                .AlwaysDo([&](const SyncConnectionString&, const std::unique_ptr<LoginCredentials>&) { return std::make_unique<FakeSyncService>(serverCases); });

            SyncClient sync_client(myDeviceId, std::unique_ptr<ISyncServiceFactory>(&mockSyncServiceFactory.get()));
            sync_client.SetSyncListener(std::make_unique<SyncLogSyncListener>());

            TestRepoBuilder repoBuilder(myDeviceId, dictionary.get());
            ISyncableDataRepository* pRepo = repoBuilder.GetRepo();

            VectorClock clientCaseClock;
            clientCaseClock.increment(serverDeviceId);
            std::shared_ptr<Case> clientUpdate1 = CreateCase(*case_access, "guid1", 1, { "clientdata1" });
            clientUpdate1->SetVectorClock(clientCaseClock);
            std::shared_ptr<Case> clientUpdate2 = CreateCase(*case_access, "guid2", 2, { "clientdata2" });
            clientUpdate2->SetVectorClock(clientCaseClock);
            repoBuilder.setInitialRepoCases({ clientUpdate1, clientUpdate2}, myDeviceId);

            sync_client.ConnectCSWeb(hostUrl, std::make_unique<LoginCredentials>(username, password));
            const SyncClient::SyncResult result = sync_client.SyncData(SyncDirection::Both, *pRepo, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            repoBuilder.verifyCaseInRepo(*serverCase1);
            repoBuilder.verifyCaseInRepo(*serverCase2);
        }


        TEST_METHOD(TestSyncClientMoreRecent)
        {
            // Make some cases to test with
            std::vector<std::shared_ptr<Case>> serverCases;
            VectorClock serverCaseClock;
            serverCaseClock.increment(serverDeviceId);
            std::shared_ptr<Case> serverCase1 = CreateCase(*case_access, "guid1", 1, { "serverdata1" });
            serverCase1->SetVectorClock(serverCaseClock);
            std::shared_ptr<Case> serverCase2 = CreateCase(*case_access, "guid2", 1, { "serverdata2" });
            serverCase2->SetVectorClock(serverCaseClock);
            serverCases.emplace_back(serverCase1);
            serverCases.emplace_back(serverCase2);

            Mock<ISyncServiceFactory> mockSyncServiceFactory;
            When(OverloadedMethod(mockSyncServiceFactory, CreateCSWebSyncService, std::unique_ptr<ISyncService>(SyncConnectionString, std::unique_ptr<LoginCredentials>)))
                .AlwaysDo([&](const SyncConnectionString&, const std::unique_ptr<LoginCredentials>&) { return std::make_unique<FakeSyncService>(serverCases); });

            SyncClient sync_client(myDeviceId, std::unique_ptr<ISyncServiceFactory>(&mockSyncServiceFactory.get()));
            sync_client.SetSyncListener(std::make_unique<SyncLogSyncListener>());

            TestRepoBuilder repoBuilder(myDeviceId, dictionary.get());
            ISyncableDataRepository* pRepo = repoBuilder.GetRepo();

            VectorClock clientCaseClock;
            clientCaseClock.increment(serverDeviceId);
            clientCaseClock.increment(myDeviceId);
            std::shared_ptr<Case> clientCase1 = CreateCase(*case_access, "guid1", 1, { "clientdata1" });
            clientCase1->SetVectorClock(clientCaseClock);
            std::shared_ptr<Case> clientCase2 = CreateCase(*case_access, "guid2", 2, { "clientdata2" });
            clientCase2->SetVectorClock(clientCaseClock);
            repoBuilder.setInitialRepoCases({ clientCase1, clientCase2 }, myDeviceId);

            sync_client.ConnectCSWeb(hostUrl, std::make_unique<LoginCredentials>(username, password));
            const SyncClient::SyncResult result = sync_client.SyncData(SyncDirection::Both, *pRepo, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            repoBuilder.verifyCaseInRepo(*clientCase1);
            repoBuilder.verifyCaseInRepo(*clientCase2);
        }


        TEST_METHOD(TestSyncConflictClientWins)
        {
            // Make some cases to test with
            std::vector<std::shared_ptr<Case>> serverCases;
            VectorClock serverCaseClock;
            serverCaseClock.increment(serverDeviceId);
            serverCaseClock.increment(serverDeviceId);
            serverCaseClock.increment(myDeviceId);
            std::shared_ptr<Case> serverCase1 = CreateCase(*case_access, "guid1", 1, { "serverdata1" });
            serverCase1->SetVectorClock(serverCaseClock);
            serverCases.emplace_back(serverCase1);
            std::shared_ptr<Case> serverCase2 = CreateCase(*case_access, "guid2", 2, { "serverdata2" });
            serverCase2->SetVectorClock(serverCaseClock);
            serverCases.emplace_back(serverCase2);

            Mock<ISyncServiceFactory> mockSyncServiceFactory;
            When(OverloadedMethod(mockSyncServiceFactory, CreateCSWebSyncService, std::unique_ptr<ISyncService>(SyncConnectionString, std::unique_ptr<LoginCredentials>)))
                .AlwaysDo([&](const SyncConnectionString&, const std::unique_ptr<LoginCredentials>&) { return std::make_unique<FakeSyncService>(serverCases); });

            SyncClient sync_client(myDeviceId, std::unique_ptr<ISyncServiceFactory>(&mockSyncServiceFactory.get()));
            sync_client.SetSyncListener(std::make_unique<SyncLogSyncListener>());

            TestRepoBuilder repoBuilder(myDeviceId, dictionary.get());
            ISyncableDataRepository* pRepo = repoBuilder.GetRepo();

            VectorClock clientCaseClock;
            clientCaseClock.increment(serverDeviceId);
            clientCaseClock.increment(myDeviceId);
            clientCaseClock.increment(myDeviceId);
            std::shared_ptr<Case> clientCase1 = CreateCase(*case_access, "guid1", 1, { "clientdata1" });
            clientCase1->SetVectorClock(clientCaseClock);
            std::shared_ptr<Case> clientCase2 = CreateCase(*case_access, "guid2", 2, { "clientdata2" });
            clientCase2->SetVectorClock(clientCaseClock);
            std::vector<std::shared_ptr<Case>> clientCases;
            clientCases.emplace_back(clientCase1);
            clientCases.emplace_back(clientCase2);
            repoBuilder.setInitialRepoCases(clientCases, myDeviceId);

            sync_client.ConnectCSWeb(hostUrl, std::make_unique<LoginCredentials>(username, password));
            const SyncClient::SyncResult result = sync_client.SyncData(SyncDirection::Both, *pRepo, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            repoBuilder.verifyCaseInRepo(*clientCase1);
            repoBuilder.verifyCaseInRepo(*clientCase2);
        }


        TEST_METHOD(TestSyncOnlyModifiedCases)
        {
            // Make some cases to test with
            std::vector<std::shared_ptr<Case>> serverCases;
            auto fakeSyncServiceFactoryUniquePtr = std::make_unique<FakeSyncServiceFactory>(serverCases);
            FakeSyncServiceFactory& fakeSyncServiceFactory = *fakeSyncServiceFactoryUniquePtr;

            TestRepoBuilder repoBuilder(myDeviceId, dictionary.get());
            ISyncableDataRepository* pRepo = repoBuilder.GetRepo();

            VectorClock clientCaseClock;
            clientCaseClock.increment(myDeviceId);
            std::shared_ptr<Case> clientCase1 = CreateCase(*case_access, "guid1", 1, { "clientdata1" });
            std::shared_ptr<Case> clientCase2 = CreateCase(*case_access, "guid2", 2, { "clientdata2" });
            std::vector<std::shared_ptr<Case>> clientCases;
            clientCases.emplace_back(clientCase1);
            clientCases.emplace_back(clientCase2);
            repoBuilder.setInitialRepoCases(clientCases, myDeviceId);

            SyncClient sync_client(myDeviceId, std::move(fakeSyncServiceFactoryUniquePtr));
            sync_client.SetSyncListener(std::make_unique<SyncLogSyncListener>());

            // First sync - should send both cases
            sync_client.ConnectCSWeb(hostUrl, std::make_unique<LoginCredentials>(username, password));
            SyncClient::SyncResult result = sync_client.SyncData(SyncDirection::Both, *pRepo, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(size_t(2), fakeSyncServiceFactory.getServer()->GetNumberOfCasesInLastPutCases());

            // Sync again - should send nothing since no mods
            result = sync_client.SyncData(SyncDirection::Both, *pRepo, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(size_t(0), fakeSyncServiceFactory.getServer()->GetNumberOfCasesInLastPutCases());

            // Modify a case and sync again - should send only modified case
            clientCaseClock.increment(myDeviceId);
            repoBuilder.updateRepoCase(clientCase1->GetUuid(), 1, "newdata1");
            result = sync_client.SyncData(SyncDirection::Both, *pRepo, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(size_t(1), fakeSyncServiceFactory.getServer()->GetNumberOfCasesInLastPutCases());
        }


        TEST_METHOD(TestSyncDataDirection)
        {
            // Make some cases to test with
            std::vector<std::shared_ptr<Case>> serverCases;
            VectorClock serverCaseClock;
            serverCaseClock.increment(serverDeviceId);
            std::shared_ptr<Case> serverCase1 = CreateCase(*case_access, "guids1", 1, { "serverdata1" });
            serverCases.emplace_back(serverCase1);
            std::shared_ptr<Case> serverCase2 = CreateCase(*case_access, "guids2", 2, { "serverdata2" });
            serverCases.emplace_back(serverCase2);

            auto fakeSyncServiceFactoryUniquePtr = std::make_unique<FakeSyncServiceFactory>(serverCases);
            FakeSyncServiceFactory& fakeSyncServiceFactory = *fakeSyncServiceFactoryUniquePtr;

            TestRepoBuilder repoBuilder(myDeviceId, dictionary.get());
            ISyncableDataRepository* pRepo = repoBuilder.GetRepo();

            VectorClock clientCaseClock;
            clientCaseClock.increment(myDeviceId);
            std::shared_ptr<Case> clientCase1 = CreateCase(*case_access, "guidc1", 1, { "11clientdata1" });
            clientCase1->SetVectorClock(clientCaseClock);
            std::shared_ptr<Case> clientCase2 = CreateCase(*case_access, "guidc2", 2, { "12clientdata2" });
            clientCase2->SetVectorClock(clientCaseClock);
            std::vector<std::shared_ptr<Case>> clientCases;
            clientCases.emplace_back(clientCase1);
            clientCases.emplace_back(clientCase2);
            repoBuilder.setInitialRepoCases(clientCases, myDeviceId);

            SyncClient sync_client(myDeviceId, std::move(fakeSyncServiceFactoryUniquePtr));
            sync_client.SetSyncListener(std::make_unique<SyncLogSyncListener>());

            sync_client.ConnectCSWeb(hostUrl, std::make_unique<LoginCredentials>(username, password));

            // Sync with put - should add cases to server but none to client
            SyncClient::SyncResult result = sync_client.SyncData(SyncDirection::Put, *pRepo, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(size_t(2), fakeSyncServiceFactory.getServer()->GetNumberOfCasesInLastPutCases());
            Assert::AreEqual(size_t(2), pRepo->GetNumberCases());

            // Sync with get - should cause no change in server cases (despite new case3 added), add two cases to client
            repoBuilder.addRepoCase("guidc3", 3, "clientdata3");
            result = sync_client.SyncData(SyncDirection::Get, *pRepo, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(5, (int) pRepo->GetNumberCases());

            // Sync with put again - should now send only the modified case
            result = sync_client.SyncData(SyncDirection::Put, *pRepo, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(size_t(1), fakeSyncServiceFactory.getServer()->GetNumberOfCasesInLastPutCases());
            Assert::AreEqual(size_t(5), pRepo->GetNumberCases());

            auto disconnectResult = sync_client.Disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, disconnectResult);
        }


        TEST_METHOD(TestSyncDataClientLosesRevisionHistoryOnGet)
        {
            // Make some cases to test with
            std::vector<std::shared_ptr<Case>> serverCases;
            VectorClock serverCaseClock;
            serverCaseClock.increment(serverDeviceId);
            std::shared_ptr<Case> serverCase1 = CreateCase(*case_access, "guids1", 1, { "serverdata1" });
            serverCase1->SetVectorClock(serverCaseClock);
            serverCases.emplace_back(serverCase1);
            std::shared_ptr<Case> serverCase2 = CreateCase(*case_access, "guids2", 2, { "serverdata2" });
            serverCase2->SetVectorClock(serverCaseClock);
            serverCases.emplace_back(serverCase2);

            Mock<ISyncService> mockServer;
            NetworkDataChunk dataChunk;
            When(Method(mockServer, Connect)).Return(std::make_shared<ConnectResponse>(serverDeviceId));
            When(Method(mockServer, Disconnect)).AlwaysReturn();
            When(Method(mockServer, SetSyncListener)).AlwaysReturn();
            When(Method(mockServer, GetSharedSyncListener)).AlwaysReturn(nullptr);
            When(Method(mockServer, GetChunk)).AlwaysReturn(dataChunk);
            const std::string serverRev1 = "1";
            const std::string serverRev2 = "2";
            When(Method(mockServer, GetCases)).
                Do([&serverCases, serverRev1](std::shared_ptr<const CaseAccess> /*case_access*/,
                                              const DeviceId& /*device_id*/, const std::string& /*universe*/, const std::string& last_server_revision,
                                              const std::string& /*last_case_uuid*/, const std::vector<std::string>& /*excluded_revisions*/) {
                Assert::IsTrue(last_server_revision.empty(), L"First sync doesn't have empty server revision");
                return SyncGetResponse(SyncGetResponse::SyncGetResult::Complete, std::make_unique<CaseObservable>(rxcpp::observable<>::iterate(serverCases)), serverRev1);
            }).Do([serverRev1](std::shared_ptr<const CaseAccess> /*case_access*/,
                               const DeviceId& /*device_id*/, const std::string& /*universe*/, const std::string& last_server_revision,
                               const std::string& /*last_case_uuid*/, const std::vector<std::string>& /*excluded_revisions*/) {
                Assert::AreEqual(serverRev1, last_server_revision, L"Second sync doesn't have revision from first sync");
                return SyncGetResponse(SyncGetResponse::SyncGetResult::RevisionNotFound);
            }).Do([&serverCases, serverRev2](std::shared_ptr<const CaseAccess> /*case_access*/,
                                            const DeviceId& /*device_id*/, const std::string& /*universe*/, const std::string& last_server_revision,
                                            const std::string& /*last_case_uuid*/, const std::vector<std::string>& /*excluded_revisions*/) {
                Assert::IsTrue(last_server_revision.empty(), L"Sync after revision not found doesn't have empty server revision");
                return SyncGetResponse(SyncGetResponse::SyncGetResult::Complete, std::make_unique<CaseObservable>(rxcpp::observable<>::iterate(serverCases)), serverRev2);
            });

            TestRepoBuilder repoBuilder(myDeviceId, dictionary.get());
            ISyncableDataRepository* pRepo = repoBuilder.GetRepo();

            Mock<ISyncServiceFactory> mockSyncServiceFactory;
            When(OverloadedMethod(mockSyncServiceFactory, CreateCSWebSyncService, std::unique_ptr<ISyncService>(SyncConnectionString, std::unique_ptr<LoginCredentials>)))
                .AlwaysDo([&](const SyncConnectionString&, const std::unique_ptr<LoginCredentials>&) { return std::unique_ptr<ISyncService>(&(mockServer.get())); });

            SyncClient sync_client(myDeviceId, std::unique_ptr<ISyncServiceFactory>(&mockSyncServiceFactory.get()));
            sync_client.SetSyncListener(std::make_unique<SyncLogSyncListener>());

            sync_client.ConnectCSWeb(hostUrl, std::make_unique<LoginCredentials>(username, password));

            // First sync, should get two cases back from server
            SyncClient::SyncResult result = sync_client.SyncData(SyncDirection::Get, *pRepo, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            result = sync_client.SyncData(SyncDirection::Get, *pRepo, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            auto disconnectResult = sync_client.Disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, disconnectResult);

            Verify(Method(mockServer, GetCases)).Exactly(3);
        }


        TEST_METHOD(TestSyncDataClientLosesRevisionHistoryOnPut)
        {
            Mock<ISyncService> mockServer;
            NetworkDataChunk dataChunk;
            When(Method(mockServer, Connect)).Return(std::make_shared<ConnectResponse>(serverDeviceId));
            When(Method(mockServer, Disconnect)).AlwaysReturn();
            When(Method(mockServer, SetSyncListener)).AlwaysReturn();
            When(Method(mockServer, GetSharedSyncListener)).AlwaysReturn(nullptr);
            When(Method(mockServer, GetChunk)).AlwaysReturn(dataChunk);
            const std::string serverRev1 = "1";
            const std::string serverRev2 = "2";
            When(Method(mockServer, PutCases)).
                Do([serverRev1](std::shared_ptr<const CaseAccess> /*case_access*/,
                                const cs::span<const Case* const> /*cases*/, const SyncBinaryDataUploadManager* /*sync_binary_data_upload_manager*/,
                                const DeviceId& /*device_id*/, const std::string& /*universe*/, const std::string& last_server_revision) {
                Assert::IsTrue(last_server_revision.empty(), L"First sync doesn't have empty server revision");
                return SyncPutResponse(SyncPutResponse::SyncPutResult::Complete, serverRev1);
            }).Do([serverRev1](std::shared_ptr<const CaseAccess> /*case_access*/,
                               const cs::span<const Case* const> /*cases*/, const SyncBinaryDataUploadManager* /*sync_binary_data_upload_manager*/,
                               const DeviceId& /*device_id*/, const std::string& /*universe*/, const std::string& last_server_revision) {
                Assert::AreEqual(serverRev1, last_server_revision, L"Second sync doesn't have revision from first sync");
                return SyncPutResponse(SyncPutResponse::SyncPutResult::RevisionNotFound);
            }).Do([serverRev2](std::shared_ptr<const CaseAccess> /*case_access*/,
                               const cs::span<const Case* const> /*cases*/, const SyncBinaryDataUploadManager* /*sync_binary_data_upload_manager*/,
                               const DeviceId& /*device_id*/, const std::string& /*universe*/, const std::string& last_server_revision) {
                Assert::IsTrue(last_server_revision.empty(), L"Sync after revision not found doesn't have empty server revision");
                return SyncPutResponse(SyncPutResponse::SyncPutResult::Complete, serverRev2);
            });

            TestRepoBuilder repoBuilder(myDeviceId, dictionary.get());
            ISyncableDataRepository* pRepo = repoBuilder.GetRepo();

            Mock<ISyncServiceFactory> mockSyncServiceFactory;
            When(OverloadedMethod(mockSyncServiceFactory, CreateCSWebSyncService, std::unique_ptr<ISyncService>(SyncConnectionString, std::unique_ptr<LoginCredentials>)))
                .AlwaysDo([&](const SyncConnectionString&, const std::unique_ptr<LoginCredentials>&) { return std::unique_ptr<ISyncService>(&(mockServer.get())); });

            SyncClient sync_client(myDeviceId, std::unique_ptr<ISyncServiceFactory>(&mockSyncServiceFactory.get()));
            sync_client.SetSyncListener(std::make_unique<SyncLogSyncListener>());

            sync_client.ConnectCSWeb(hostUrl, std::make_unique<LoginCredentials>(username, password));

            // First sync, should get two cases back from server
            SyncClient::SyncResult result = sync_client.SyncData(SyncDirection::Put, *pRepo, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            result = sync_client.SyncData(SyncDirection::Put, *pRepo, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            auto disconnectResult = sync_client.Disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, disconnectResult);

            Verify(Method(mockServer, PutCases)).Exactly(3);
        }


        TEST_METHOD(TestSyncDataChunkedGet)
        {
            // Make some cases to test with
            std::vector<std::shared_ptr<Case>> serverCases;
            VectorClock serverCaseClock;
            serverCaseClock.increment(serverDeviceId);
            for (int i = 0; i < 30; ++i) {
                std::shared_ptr<Case> serverCase = CreateCase(*case_access, FormatText("guid%02d", i), 1, { "data" });
                serverCase->SetVectorClock(serverCaseClock);
                serverCases.emplace_back(serverCase);
            }

            Mock<ISyncService> mockServer;
            NetworkDataChunk dataChunk;
            When(Method(mockServer, Connect)).Return(std::make_shared<ConnectResponse>(serverDeviceId));
            When(Method(mockServer, Disconnect)).AlwaysReturn();
            When(Method(mockServer, SetSyncListener)).AlwaysReturn();
            When(Method(mockServer, GetSharedSyncListener)).AlwaysReturn(nullptr);
            When(Method(mockServer, GetChunk)).AlwaysReturn(dataChunk);
            const std::string serverRev1 = "1";
            const std::string serverRev2 = "2";
            const std::string serverRev3 = "3";
            When(Method(mockServer, GetCases)).
                Do([&serverCases, serverRev1](std::shared_ptr<const CaseAccess> /*case_access*/,
                                              const DeviceId& /*device_id*/, const std::string& /*universe*/, const std::string& last_server_revision,
                                              const std::string& last_case_uuid, const std::vector<std::string>& /*excluded_revisions*/) -> SyncGetResponse {
                    // First sync, first chunk fails
                    Assert::IsTrue(last_server_revision.empty(), L"First sync doesn't have empty server revision");
                    Assert::IsTrue(last_case_uuid.empty(), L"First sync doesn't have empty last uuid");
                    throw SyncError(100111, "some network error");
                }).Do([&serverCases, serverRev1](std::shared_ptr<const CaseAccess> /*case_access*/,
                                                 const DeviceId& /*device_id*/, const std::string& /*universe*/, const std::string& last_server_revision,
                                                 const std::string& last_case_uuid, const std::vector<std::string>& /*excluded_revisions*/) {
                    // Second sync, first chunk
                    std::vector<std::shared_ptr<Case>> first10;
                    std::copy(serverCases.begin(), serverCases.begin() + 10, std::back_inserter(first10));
                    Assert::IsTrue(last_server_revision.empty(), L"First sync doesn't have empty server revision");
                    Assert::IsTrue(last_case_uuid.empty(), L"First sync doesn't have empty last uuid");
                    return SyncGetResponse(SyncGetResponse::SyncGetResult::MoreData, std::make_unique<CaseObservable>(rxcpp::observable<>::iterate(first10)), serverRev1);
                }).Do([&serverCases, serverRev1](std::shared_ptr<const CaseAccess> /*case_access*/,
                                                 const DeviceId& /*device_id*/, const std::string& /*universe*/, const std::string& last_server_revision,
                                                 const std::string& last_case_uuid, const std::vector<std::string>& /*excluded_revisions*/) -> SyncGetResponse {
                    // Second sync, second chunk
                    Assert::AreEqual(serverRev1, last_server_revision);
                    Assert::AreEqual(serverCases[9]->GetUuid(), last_case_uuid);
                    throw SyncError(100111, "some network error");
                }).Do([&serverCases, serverRev1, serverRev2](std::shared_ptr<const CaseAccess> /*case_access*/,
                                                             const DeviceId& /*device_id*/, const std::string& /*universe*/, const std::string& last_server_revision,
                                                             const std::string& last_case_uuid, const std::vector<std::string>& /*excluded_revisions*/) {
                    // Third sync, second chunk
                    Assert::AreEqual(serverRev1, last_server_revision);
                    Assert::AreEqual(serverCases[9]->GetUuid(), last_case_uuid);
                    std::vector<std::shared_ptr<Case>> second10;
                    std::copy(serverCases.begin() + 10, serverCases.begin() + 20, std::back_inserter(second10));
                    return SyncGetResponse(SyncGetResponse::SyncGetResult::MoreData, std::make_unique<CaseObservable>(rxcpp::observable<>::iterate(second10)), serverRev2);
                }).Do([&serverCases, serverRev2, serverRev3](std::shared_ptr<const CaseAccess> /*case_access*/,
                                                             const DeviceId& /*device_id*/, const std::string& /*universe*/, const std::string& last_server_revision,
                                                             const std::string& last_case_uuid, const std::vector<std::string>& /*excluded_revisions*/) -> SyncGetResponse {
                    // Third sync, third chunk
                    Assert::AreEqual(serverRev2, last_server_revision);
                    Assert::AreEqual(serverCases[19]->GetUuid(), last_case_uuid);
                    throw SyncError(100111, "some network error");
                }).Do([&serverCases, serverRev2, serverRev3](std::shared_ptr<const CaseAccess> /*case_access*/,
                                                             const DeviceId& /*device_id*/, const std::string& /*universe*/, const std::string& last_server_revision,
                                                             const std::string& last_case_uuid, const std::vector<std::string>& /*excluded_revisions*/) {
                    // Fourth sync, third chunk
                    Assert::AreEqual(serverRev2, last_server_revision);
                    Assert::AreEqual(serverCases[19]->GetUuid(), last_case_uuid);
                    std::vector<std::shared_ptr<Case>> last10;
                    std::copy(serverCases.begin() + 20, serverCases.end(), std::back_inserter(last10));
                    return SyncGetResponse(SyncGetResponse::SyncGetResult::Complete, std::make_unique<CaseObservable>(rxcpp::observable<>::iterate(last10)), serverRev3);
                });

            TestRepoBuilder repoBuilder(myDeviceId, dictionary.get());
            ISyncableDataRepository* pRepo = repoBuilder.GetRepo();

            Mock<ISyncServiceFactory> mockSyncServiceFactory;
            When(OverloadedMethod(mockSyncServiceFactory, CreateCSWebSyncService, std::unique_ptr<ISyncService>(SyncConnectionString, std::unique_ptr<LoginCredentials>)))
                .AlwaysDo([&](const SyncConnectionString&, const std::unique_ptr<LoginCredentials>&) { return std::unique_ptr<ISyncService>(&(mockServer.get())); });

            SyncClient sync_client(myDeviceId, std::unique_ptr<ISyncServiceFactory>(&mockSyncServiceFactory.get()));
            sync_client.SetSyncListener(std::make_unique<SyncLogSyncListener>());

            sync_client.ConnectCSWeb(hostUrl, std::make_unique<LoginCredentials>(username, password));

            // First sync gets nothing
            SyncClient::SyncResult result = sync_client.SyncData(SyncDirection::Get, *pRepo, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_ERROR, result);
            Assert::AreEqual(0, (int) pRepo->GetNumberCases());

            // Second sync should get 10 cases back from server with error
            result = sync_client.SyncData(SyncDirection::Get, *pRepo, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_ERROR, result);
            Assert::AreEqual(10, (int) pRepo->GetNumberCases());
            for (int i = 0; i < 10; ++i)
                repoBuilder.verifyCaseInRepo(*serverCases[i]);

            // Third should get 10 more cases back from server with error
            result = sync_client.SyncData(SyncDirection::Get, *pRepo, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_ERROR, result);
            Assert::AreEqual(20, (int) pRepo->GetNumberCases());
            for (int i = 0; i < 20; ++i)
                repoBuilder.verifyCaseInRepo(*serverCases[i]);

            // Fourth sync should get 10 more cases back from server with no error
            result = sync_client.SyncData(SyncDirection::Get, *pRepo, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(30, (int) pRepo->GetNumberCases());
            for (size_t i = 0; i < serverCases.size(); ++i)
                repoBuilder.verifyCaseInRepo(*serverCases[i]);

            auto disconnectResult = sync_client.Disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, disconnectResult);

            Verify(Method(mockServer, GetCases)).Exactly(6);
        }


        TEST_METHOD(TestSyncDataChunkedPut)
        {
            const int numCases = 25;
            const size_t lastChunkSize = 5;

            // Make some cases to test with
            std::vector<std::shared_ptr<Case>> clientCases;
            VectorClock clock;
            clock.increment(myDeviceId);
            for (int i = 0; i < numCases; ++i) {
                std::shared_ptr<Case> clientCase = CreateCase(*case_access, FormatText("guid%02d", i), i, { "data" });
                clientCase->SetVectorClock(clock);
                clientCases.emplace_back(clientCase);
            }

            Mock<ISyncService> mockServer;
            NetworkDataChunk dataChunk(10);
            const size_t chunkSize = dataChunk.GetCaseSize();
            When(Method(mockServer, Connect)).Return(std::make_shared<ConnectResponse>(serverDeviceId));
            When(Method(mockServer, Disconnect)).AlwaysReturn();
            When(Method(mockServer, SetSyncListener)).AlwaysReturn();
            When(Method(mockServer, GetSharedSyncListener)).AlwaysReturn(nullptr);
            When(Method(mockServer, GetChunk)).AlwaysReturn(dataChunk);
            const std::string serverRev1 = "1";
            const std::string serverRev2 = "2";
            const std::string serverRev3 = "3";
            When(Method(mockServer, PutCases)).
                Do([&clientCases, chunkSize, this](std::shared_ptr<const CaseAccess> /*case_access*/,
                                                   const cs::span<const Case* const> cases, const SyncBinaryDataUploadManager* /*sync_binary_data_upload_manager*/,
                                                   const DeviceId& /*device_id*/, const std::string& /*universe*/, const std::string& last_server_revision) -> SyncPutResponse {
                // First sync first chunk
                Assert::IsTrue(last_server_revision.empty(), L"First sync doesn't have empty server revision");
                Assert::AreEqual(chunkSize, cases.size());
                Assert::AreEqual(clientCases[0]->GetUuid(), cases.front()->GetUuid());
                throw SyncError(100111, "some network error");
            }).Do([&clientCases, this, serverRev1, chunkSize](std::shared_ptr<const CaseAccess> /*case_access*/,
                                                              const cs::span<const Case* const> cases, const SyncBinaryDataUploadManager* /*sync_binary_data_upload_manager*/,
                                                              const DeviceId& /*device_id*/, const std::string& /*universe*/, const std::string& last_server_revision) {
                // Second sync first chunk retry
                Assert::AreEqual(chunkSize, cases.size());
                Assert::AreEqual(clientCases[0]->GetUuid(), cases.front()->GetUuid());
                Assert::IsTrue(last_server_revision.empty(), L"First sync doesn't have empty server revision");
                return SyncPutResponse(SyncPutResponse::SyncPutResult::Complete, serverRev1);
            }).Do([&clientCases, this, serverRev1, chunkSize](std::shared_ptr<const CaseAccess> /*case_access*/,
                                                              const cs::span<const Case* const> cases, const SyncBinaryDataUploadManager* /*sync_binary_data_upload_manager*/,
                                                              const DeviceId& /*device_id*/, const std::string& /*universe*/, const std::string& last_server_revision) -> SyncPutResponse {
                // Second sync second chunk
                Assert::AreEqual(serverRev1, last_server_revision);
                Assert::AreEqual(chunkSize, cases.size());
                Assert::AreEqual(clientCases[chunkSize]->GetUuid(), cases.front()->GetUuid());
                throw SyncError(100111, "some network error");
            }).Do([&clientCases, this, serverRev1, serverRev2, chunkSize](std::shared_ptr<const CaseAccess> /*case_access*/,
                                                                          const cs::span<const Case* const> cases, const SyncBinaryDataUploadManager* /*sync_binary_data_upload_manager*/,
                                                                          const DeviceId& /*device_id*/, const std::string& /*universe*/, const std::string& last_server_revision) {
                // Third sync second chunk retry
                Assert::AreEqual(serverRev1, last_server_revision);
                Assert::AreEqual(chunkSize, cases.size());
                Assert::AreEqual(clientCases[chunkSize]->GetUuid(), cases.front()->GetUuid());
                return SyncPutResponse(SyncPutResponse::SyncPutResult::Complete, serverRev2);
            }).Do([&clientCases, this, serverRev2, serverRev3, chunkSize, lastChunkSize](std::shared_ptr<const CaseAccess> /*case_access*/,
                                                                                         const cs::span<const Case* const> cases, const SyncBinaryDataUploadManager* /*sync_binary_data_upload_manager*/,
                                                                                         const DeviceId& /*device_id*/, const std::string& /*universe*/, const std::string& last_server_revision) {
                // Third sync third chunk
                Assert::AreEqual(serverRev2, last_server_revision);
                Assert::AreEqual(lastChunkSize, cases.size());
                Assert::AreEqual(clientCases[2 * chunkSize]->GetUuid(), cases.front()->GetUuid());
                return SyncPutResponse(SyncPutResponse::SyncPutResult::Complete, serverRev3);
            }).Do([&clientCases, this, serverRev3](std::shared_ptr<const CaseAccess> /*case_access*/,
                                                   const cs::span<const Case* const> cases, const SyncBinaryDataUploadManager* /*sync_binary_data_upload_manager*/,
                                                   const DeviceId& /*device_id*/, const std::string& /*universe*/, const std::string& last_server_revision) {
                // Fourth sync
                Assert::AreEqual(serverRev3, last_server_revision);
                Assert::AreEqual(size_t(0), cases.size());
                return SyncPutResponse(SyncPutResponse::SyncPutResult::Complete, serverRev3);
            });

            TestRepoBuilder repoBuilder(myDeviceId, dictionary.get());
            ISyncableDataRepository* pRepo = repoBuilder.GetRepo();
            repoBuilder.setInitialRepoCases(clientCases, myDeviceId);

            Mock<ISyncServiceFactory> mockSyncServiceFactory;
            When(OverloadedMethod(mockSyncServiceFactory, CreateCSWebSyncService, std::unique_ptr<ISyncService>(SyncConnectionString, std::unique_ptr<LoginCredentials>)))
                .AlwaysDo([&](const SyncConnectionString&, const std::unique_ptr<LoginCredentials>&) { return std::unique_ptr<ISyncService>(&(mockServer.get())); });

            SyncClient sync_client(myDeviceId, std::unique_ptr<ISyncServiceFactory>(&mockSyncServiceFactory.get()));
            sync_client.SetSyncListener(std::make_unique<SyncLogSyncListener>());

            sync_client.ConnectCSWeb(hostUrl, std::make_unique<LoginCredentials>(username, password));

            // First sync uploads nothing - gets error
            SyncClient::SyncResult result = sync_client.SyncData(SyncDirection::Put, *pRepo, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_ERROR, result);

            // Second sync should put 10 cases with error
            result = sync_client.SyncData(SyncDirection::Put, *pRepo, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_ERROR, result);

            // Third sync should put the remaining 15 cases
            result = sync_client.SyncData(SyncDirection::Put, *pRepo, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            // Fourth sync should upload nothing since everything was already uploaded
            result = sync_client.SyncData(SyncDirection::Put, *pRepo, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);

            auto disconnectResult = sync_client.Disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, disconnectResult);

            Verify(Method(mockServer, PutCases)).Exactly(6);
        }


        TEST_METHOD(TestSyncDataDontUploadDownloadedCases)
        {
            // Make some cases to test with
            std::vector<std::shared_ptr<Case>> serverCases;
            VectorClock serverCaseClock;
            serverCaseClock.increment(serverDeviceId);
            for (int i = 0; i < 10; ++i) {
                std::shared_ptr<Case> serverCase = CreateCase(*case_access, CreateUuid(), 1, { "data" });
                serverCase->SetVectorClock(serverCaseClock);
                serverCases.emplace_back(serverCase);
            }

            std::vector<std::shared_ptr<Case>> updatedServerCases;

            Mock<ISyncService> mockServer;
            NetworkDataChunk dataChunk;
            When(Method(mockServer, Connect)).Return(std::make_shared<ConnectResponse>(serverDeviceId));
            When(Method(mockServer, Disconnect)).AlwaysReturn();
            When(Method(mockServer, SetSyncListener)).AlwaysReturn();
            When(Method(mockServer, GetSharedSyncListener)).AlwaysReturn(nullptr);
            When(Method(mockServer, GetChunk)).AlwaysReturn(dataChunk);
            int serverRev = 1;
            When(Method(mockServer, GetCases)).
            Do([&serverCases, &serverRev](std::shared_ptr<const CaseAccess> /*case_access*/,
                                          const DeviceId& /*device_id*/, const std::string& /*universe*/, const std::string& /*last_server_revision*/,
                                          const std::string& /*last_case_uuid*/, const std::vector<std::string>& /*excluded_revisions*/) {
                return SyncGetResponse(SyncGetResponse::SyncGetResult::Complete, std::make_unique<CaseObservable>(rxcpp::observable<>::iterate(serverCases)), IntToString(serverRev));
            }).Do([&updatedServerCases, &serverRev](std::shared_ptr<const CaseAccess> /*case_access*/,
                                                    const DeviceId& /*device_id*/, const std::string& /*universe*/, const std::string& /*last_server_revision*/,
                                                    const std::string& /*last_case_uuid*/, const std::vector<std::string>& /*excluded_revisions*/) {
                return SyncGetResponse(SyncGetResponse::SyncGetResult::Complete, std::make_unique<CaseObservable>(rxcpp::observable<>::iterate(updatedServerCases)), IntToString(serverRev));
            }).Do([&serverRev](std::shared_ptr<const CaseAccess> /*case_access*/,
                               const DeviceId& /*device_id*/, const std::string& /*universe*/, const std::string& /*last_server_revision*/,
                               const std::string& /*last_case_uuid*/, const std::vector<std::string>& /*excluded_revisions*/) {
                return SyncGetResponse(SyncGetResponse::SyncGetResult::Complete, std::make_unique<CaseObservable>(rxcpp::observable<>::iterate(std::vector<std::shared_ptr<Case>>())), IntToString(serverRev));
            }).Do([](std::shared_ptr<const CaseAccess> /*case_access*/,
                     const DeviceId& /*device_id*/, const std::string& /*universe*/, const std::string& /*last_server_revision*/,
                     const std::string& /*last_case_uuid*/, const std::vector<std::string>& /*excluded_revisions*/) {
                return SyncGetResponse(SyncGetResponse::SyncGetResult::RevisionNotFound);
            }).Do([&serverRev](std::shared_ptr<const CaseAccess> /*case_access*/,
                               const DeviceId& /*device_id*/, const std::string& /*universe*/, const std::string& /*last_server_revision*/,
                               const std::string& /*last_case_uuid*/, const std::vector<std::string>& /*excluded_revisions*/) {
                return SyncGetResponse(SyncGetResponse::SyncGetResult::Complete, std::make_unique<CaseObservable>(rxcpp::observable<>::iterate(std::vector<std::shared_ptr<Case>>())), IntToString(serverRev));
            });

            When(Method(mockServer, PutCases)).
                Do([&serverRev](std::shared_ptr<const CaseAccess> /*case_access*/,
                                const cs::span<const Case* const> cases, const SyncBinaryDataUploadManager* /*sync_binary_data_upload_manager*/,
                                const DeviceId& /*device_id*/, const std::string& /*universe*/, const std::string& /*last_server_revision*/) {
                // First sync should put no cases
                Assert::AreEqual(size_t(0), cases.size());
                return SyncPutResponse(SyncPutResponse::SyncPutResult::Complete, IntToString(++serverRev));
            }).Do([&serverRev](std::shared_ptr<const CaseAccess> /*case_access*/,
                               const cs::span<const Case* const> cases, const SyncBinaryDataUploadManager* /*sync_binary_data_upload_manager*/,
                               const DeviceId& /*device_id*/, const std::string& /*universe*/, const std::string& /*last_server_revision*/) {
                // Second sync should put 2 cases that were added/modified on client
                Assert::AreEqual(size_t(2), cases.size());
                return SyncPutResponse(SyncPutResponse::SyncPutResult::Complete, IntToString(++serverRev));
            }).Do([&serverRev](std::shared_ptr<const CaseAccess> /*case_access*/,
                               const cs::span<const Case* const> cases, const SyncBinaryDataUploadManager* /*sync_binary_data_upload_manager*/,
                               const DeviceId& /*device_id*/, const std::string& /*universe*/, const std::string& /*last_server_revision*/) {
                // Third sync should put nothing
                Assert::AreEqual(size_t(0), cases.size());
                return SyncPutResponse(SyncPutResponse::SyncPutResult::Complete, IntToString(++serverRev));
            }).Do([](std::shared_ptr<const CaseAccess> /*case_access*/,
                     const cs::span<const Case* const> /*cases*/, const SyncBinaryDataUploadManager* /*sync_binary_data_upload_manager*/,
                     const DeviceId& /*device_id*/, const std::string& /*universe*/, const std::string& /*last_server_revision*/) {
                return SyncPutResponse(SyncPutResponse::SyncPutResult::RevisionNotFound);
            }).Do([&serverRev](std::shared_ptr<const CaseAccess> /*case_access*/,
                               const cs::span<const Case* const> cases, const SyncBinaryDataUploadManager* /*sync_binary_data_upload_manager*/,
                               const DeviceId& /*device_id*/, const std::string& /*universe*/, const std::string& /*last_server_revision*/) {
                // Third sync should upload everything
                Assert::AreEqual(size_t(12), cases.size());
                return SyncPutResponse(SyncPutResponse::SyncPutResult::Complete, IntToString(++serverRev));
            });

            TestRepoBuilder repoBuilder(myDeviceId, dictionary.get());
            ISyncableDataRepository* pRepo = repoBuilder.GetRepo();

            Mock<ISyncServiceFactory> mockSyncServiceFactory;
            When(OverloadedMethod(mockSyncServiceFactory, CreateCSWebSyncService, std::unique_ptr<ISyncService>(SyncConnectionString, std::unique_ptr<LoginCredentials>)))
                .AlwaysDo([&](const SyncConnectionString&, const std::unique_ptr<LoginCredentials>&) { return std::unique_ptr<ISyncService>(&(mockServer.get())); });

            SyncClient sync_client(myDeviceId, std::unique_ptr<ISyncServiceFactory>(&mockSyncServiceFactory.get()));
            sync_client.SetSyncListener(std::make_unique<SyncLogSyncListener>());

            sync_client.ConnectCSWeb(hostUrl, std::make_unique<LoginCredentials>(username, password));

            // First sync gets initial server cases, sends nothing to server
            SyncClient::SyncResult result = sync_client.SyncData(SyncDirection::Both, *pRepo, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(10, (int) pRepo->GetNumberCases());

            // Modify/add case on server
            std::shared_ptr<Case> updatedServerCase1 = CreateCase(*case_access, CreateUuid(), 1, { "servernew" });
            updatedServerCase1->SetVectorClock(serverCaseClock);
            updatedServerCases.emplace_back(updatedServerCase1);
            serverCaseClock.increment(serverDeviceId);
            std::shared_ptr<Case> updatedServerCase2 = CreateCase(*case_access, serverCases[1]->GetUuid(), 1, { "servermod" });
            updatedServerCase2->SetVectorClock(serverCaseClock);
            updatedServerCases.emplace_back(updatedServerCase2);

            // Modify/add case on client
            repoBuilder.addRepoCase(CreateUuid(), 1, { "addedclient" });
            repoBuilder.updateRepoCase(serverCases[0]->GetUuid(), 1, { "data" });

            // Sync again, only 2 changed cases on server downloaded, only 2 changed on client uploaded
            result = sync_client.SyncData(SyncDirection::Both, *pRepo, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(12, (int) pRepo->GetNumberCases());

            // Next sync should put/get nothing
            result = sync_client.SyncData(SyncDirection::Both, *pRepo, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(12, (int) pRepo->GetNumberCases());

            // Simulate a server reset and make sure that all cases are uploaded on next sync
            serverRev = 1;
            result = sync_client.SyncData(SyncDirection::Both, *pRepo, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, result);
            Assert::AreEqual(12, (int) pRepo->GetNumberCases());

            auto disconnectResult = sync_client.Disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, disconnectResult);
        }


        TEST_METHOD(TestSyncGetFile)
        {
            const std::string fileContents("SOME DATA");
            Mock<ISyncService> mockServer;
            When(Method(mockServer, Connect)).Return(std::make_shared<ConnectResponse>(serverDeviceId));
            When(Method(mockServer, Disconnect)).AlwaysReturn();
            When(Method(mockServer, SetSyncListener)).AlwaysReturn();
            When(Method(mockServer, GetSharedSyncListener)).AlwaysReturn(nullptr);
            When(Method(mockServer, GetFile)).AlwaysDo([&fileContents](const std::string&, const std::string& local_file_path, const std::string&) -> bool {
                FileIO::WriteText(local_file_path, fileContents, false);
                return true;
            });

            Mock<ISyncServiceFactory> mockSyncServiceFactory;
            When(OverloadedMethod(mockSyncServiceFactory, CreateCSWebSyncService, std::unique_ptr<ISyncService>(SyncConnectionString, std::unique_ptr<LoginCredentials>)))
                .AlwaysDo([&](const SyncConnectionString&, const std::unique_ptr<LoginCredentials>&) { return std::unique_ptr<ISyncService>(&(mockServer.get())); });

            SyncClient sync_client(myDeviceId, std::unique_ptr<ISyncServiceFactory>(&mockSyncServiceFactory.get()));
            sync_client.SetSyncListener(std::make_unique<SyncLogSyncListener>());

            auto connectOkResult = sync_client.ConnectCSWeb(hostUrl, std::make_unique<LoginCredentials>(username, password));
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, connectOkResult);

            // Test absolute path with filename
            const std::string destFilePath = Path::Combine(GetTempDirectory(), "zsynco-get-file-test.txt");
            PortableFunctions::FileDelete(destFilePath);
            SyncClient::SyncResult syncResult = sync_client.SyncFile(SyncDirection::Get, "/some/file/on/server", destFilePath);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, syncResult);
            Assert::IsTrue(PortableFunctions::FileIsRegular(destFilePath));
            Assert::AreEqual(fileContents, FileIO::ReadText(destFilePath));
            PortableFunctions::FileDeleteWithExceptions(destFilePath);

            // Absolute path with no filename - should use filename from source
            syncResult = sync_client.SyncFile(SyncDirection::Get, "/some/file/on/server/zsynco-get-file-test.txt", GetTempDirectory());
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, syncResult);
            Assert::IsTrue(PortableFunctions::FileIsRegular(destFilePath));
            PortableFunctions::FileDeleteWithExceptions(destFilePath);

            // To path has parent directories that don't exist - should create them
            const std::string subdirRoot = Path::Combine(GetTempDirectory(), "zsynco-get-file-test");
            const std::string destFilePathWithSubdirs = Path::Combine(subdirRoot, "directory\\that\\does\\not\\exist\\zsynco-get-file-test.txt");
            PortableFunctions::DirectoryDelete(subdirRoot, true);
            syncResult = sync_client.SyncFile(SyncDirection::Get, "/some/path/somefile", destFilePathWithSubdirs);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, syncResult);
            Assert::IsTrue(PortableFunctions::FileIsRegular(destFilePathWithSubdirs));
            PortableFunctions::DirectoryDelete(subdirRoot, true);

            auto disconnectResult = sync_client.Disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, disconnectResult);
        }


        TEST_METHOD(TestSyncGetFileError)
        {
            const std::string fileContents("SOME DATA");
            Mock<ISyncService> mockServer;
            When(Method(mockServer, Connect)).Return(std::make_shared<ConnectResponse>(serverDeviceId));
            When(Method(mockServer, Disconnect)).AlwaysReturn();
            When(Method(mockServer, SetSyncListener)).AlwaysReturn();
            When(Method(mockServer, GetSharedSyncListener)).AlwaysReturn(nullptr);
            When(Method(mockServer, GetFile)).AlwaysThrow(SyncError(100110, "filename"));

            Mock<ISyncServiceFactory> mockSyncServiceFactory;
            When(OverloadedMethod(mockSyncServiceFactory, CreateCSWebSyncService, std::unique_ptr<ISyncService>(SyncConnectionString, std::unique_ptr<LoginCredentials>)))
                .AlwaysDo([&](const SyncConnectionString&, const std::unique_ptr<LoginCredentials>&) { return std::unique_ptr<ISyncService>(&(mockServer.get())); });

            SyncClient sync_client(myDeviceId, std::unique_ptr<ISyncServiceFactory>(&mockSyncServiceFactory.get()));
            sync_client.SetSyncListener(std::make_unique<SyncLogSyncListener>());

            auto connectOkResult = sync_client.ConnectCSWeb(hostUrl, std::make_unique<LoginCredentials>(username, password));
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, connectOkResult);

            TemporaryFile temporary_file;
            PortableFunctions::FileDeleteWithExceptions(temporary_file.GetPath());

            SyncClient::SyncResult syncResult = sync_client.SyncFile(SyncDirection::Get, "/some/file/on/server", temporary_file.GetPath());
            Assert::AreEqual(SyncClient::SyncResult::SYNC_ERROR, syncResult);

            // File does not exist
            Assert::IsFalse(PortableFunctions::FileExists(temporary_file.GetPath()));

            auto disconnectResult = sync_client.Disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, disconnectResult);
        }


        TEST_METHOD(TestSyncGetFileWildcard)
        {
            const std::string fileContents("SOME DATA");
            Mock<ISyncService> mockServer;
            When(Method(mockServer, Connect)).Return(std::make_shared<ConnectResponse>(serverDeviceId));
            When(Method(mockServer, Disconnect)).AlwaysReturn();
            When(Method(mockServer, SetSyncListener)).AlwaysReturn();
            When(Method(mockServer, GetSharedSyncListener)).AlwaysReturn(nullptr);
            std::map<std::string, int> getFileCounts;
            When(Method(mockServer, GetFile)).AlwaysDo([&](const std::string& rp, const std::string& lp, const std::string&) -> bool {
                getFileCounts[rp] = getFileCounts[rp] + 1;
                std::ofstream os(lp, std::ios::binary);
                os.write(fileContents.c_str(), fileContents.size());
                return true;
            });
            std::vector<FileInfo> directory_listing;
            directory_listing.emplace_back(FileInfo::FileType::File, "foo1", "/", 10, 0, "123124124");
            directory_listing.emplace_back(FileInfo::FileType::File, "foo2", "/", 10, 0, "123124124");
            directory_listing.emplace_back(FileInfo::FileType::File, "fooa", "/", 10, 0, "123124124");
            directory_listing.emplace_back(FileInfo::FileType::File, "bar", "/", 10, 0, "123124124");
            directory_listing.emplace_back(FileInfo::FileType::Directory, "foodir", "/");
            directory_listing.emplace_back(FileInfo::FileType::File, "foothree", "/", 10, 0, "123124124");
            When(Method(mockServer, GetDirectoryListing)).AlwaysDo([directory_listing](const std::string&, bool) -> std::vector<FileInfo> {
                return directory_listing;
            });

            Mock<ISyncServiceFactory> mockSyncServiceFactory;
            When(OverloadedMethod(mockSyncServiceFactory, CreateCSWebSyncService, std::unique_ptr<ISyncService>(SyncConnectionString, std::unique_ptr<LoginCredentials>)))
                .AlwaysDo([&](const SyncConnectionString&, const std::unique_ptr<LoginCredentials>&) { return std::unique_ptr<ISyncService>(&(mockServer.get())); });

            SyncClient sync_client(myDeviceId, std::unique_ptr<ISyncServiceFactory>(&mockSyncServiceFactory.get()));
            sync_client.SetSyncListener(std::make_unique<SyncLogSyncListener>());

            auto connectOkResult = sync_client.ConnectCSWeb(hostUrl, std::make_unique<LoginCredentials>(username, password));
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, connectOkResult);

            SyncClient::SyncResult syncResult = sync_client.SyncFile(SyncDirection::Get, "/*", GetTempDirectory());
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, syncResult);

            syncResult = sync_client.SyncFile(SyncDirection::Get, "/foo*", GetTempDirectory());
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, syncResult);

            syncResult = sync_client.SyncFile(SyncDirection::Get, "/foo?", GetTempDirectory());
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, syncResult);

            //# is not recognized as the wild card in the current implementation of regex. so foo1, foo2 will not be matched
            syncResult = sync_client.SyncFile(SyncDirection::Get, "/foo#", GetTempDirectory());
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, syncResult);

            Assert::AreEqual(getFileCounts[directory_listing[0].GetDirectory() + directory_listing[0].GetName()], 3);
            Assert::AreEqual(getFileCounts[directory_listing[1].GetDirectory() + directory_listing[1].GetName()], 3);
            Assert::AreEqual(getFileCounts[directory_listing[2].GetDirectory() + directory_listing[2].GetName()], 3);
            Assert::AreEqual(getFileCounts[directory_listing[3].GetDirectory() + directory_listing[3].GetName()], 1);
            Assert::AreEqual(getFileCounts[directory_listing[4].GetDirectory() + directory_listing[4].GetName()], 0);
            Assert::AreEqual(getFileCounts[directory_listing[5].GetDirectory() + directory_listing[5].GetName()], 2);

            auto disconnectResult = sync_client.Disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, disconnectResult);
        }


        TEST_METHOD(TestSyncPutFile)
        {
            const std::string fileContents("SOME DATA");
            Mock<ISyncService> mockServer;
            When(Method(mockServer, Connect)).Return(std::make_shared<ConnectResponse>(serverDeviceId));
            When(Method(mockServer, Disconnect)).AlwaysReturn();
            When(Method(mockServer, SetSyncListener)).AlwaysReturn();
            When(Method(mockServer, GetSharedSyncListener)).AlwaysReturn(nullptr);
            std::map<std::string, int> putFileCounts;
            When(Method(mockServer, PutFile)).AlwaysDo([&](const std::string& local_file_path, const std::string& remote_path) -> void {
                putFileCounts[remote_path] = putFileCounts[remote_path] + 1;
                std::ifstream uploadStream(local_file_path.c_str(), std::ios::binary);
                std::string s(std::istreambuf_iterator<char>(uploadStream), {});
                Assert::AreEqual(fileContents, s);
            });

            Mock<ISyncServiceFactory> mockSyncServiceFactory;
            When(OverloadedMethod(mockSyncServiceFactory, CreateCSWebSyncService, std::unique_ptr<ISyncService>(SyncConnectionString, std::unique_ptr<LoginCredentials>)))
                .AlwaysDo([&](const SyncConnectionString&, const std::unique_ptr<LoginCredentials>&) { return std::unique_ptr<ISyncService>(&(mockServer.get())); });

            const std::string srcFileAbsPath = Path::Combine(GetTempDirectory(), "zsynco-put-file-test.txt");
            FileIO::WriteText(srcFileAbsPath, fileContents, false);

            SyncClient sync_client(myDeviceId, std::unique_ptr<ISyncServiceFactory>(&mockSyncServiceFactory.get()));
            sync_client.SetSyncListener(std::make_unique<SyncLogSyncListener>());

            auto connectOkResult = sync_client.ConnectCSWeb(hostUrl, std::make_unique<LoginCredentials>(username, password));
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, connectOkResult);

            // Test absolute dest path with filename
            const std::string destFilePath = "/some/folder";
            std::string fullDestFilePath = PortableFunctions::PathAppendForwardSlashToPath(destFilePath, "fileonserver.txt");
            SyncClient::SyncResult syncResult = sync_client.SyncFile(SyncDirection::Put, srcFileAbsPath, fullDestFilePath);
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, syncResult);
            Assert::AreEqual(putFileCounts[fullDestFilePath], 1);

            // Absolute dest path with no filename - should use filename from source
            fullDestFilePath = PortableFunctions::PathAppendForwardSlashToPath(destFilePath, PortableFunctions::PathGetFilename(srcFileAbsPath));
            syncResult = sync_client.SyncFile(SyncDirection::Put, srcFileAbsPath, PortableFunctions::PathGetDirectory(fullDestFilePath));
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, syncResult);
            Assert::AreEqual(putFileCounts[fullDestFilePath], 1);

            // Empty to path - should put file in file root with source name
            fullDestFilePath = PortableFunctions::PathAppendForwardSlashToPath("/", PortableFunctions::PathGetFilename(srcFileAbsPath));
            syncResult = sync_client.SyncFile(SyncDirection::Put, srcFileAbsPath, "");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, syncResult);
            Assert::AreEqual(putFileCounts[fullDestFilePath], 1);

            auto disconnectResult = sync_client.Disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, disconnectResult);
        }


        TEST_METHOD(TestSyncPutFileWildcard)
        {
            const std::string fileContents("SOME DATA");
            Mock<ISyncService> mockServer;
            When(Method(mockServer, Connect)).Return(std::make_shared<ConnectResponse>(serverDeviceId));
            When(Method(mockServer, Disconnect)).AlwaysReturn();
            When(Method(mockServer, SetSyncListener)).AlwaysReturn();
            When(Method(mockServer, GetSharedSyncListener)).AlwaysReturn(nullptr);
            std::map<std::string, int> putFileCounts;
            When(Method(mockServer, PutFile)).AlwaysDo([&](const std::string& /*local_file_path*/, const std::string& remote_path) -> void {
                putFileCounts[remote_path] = putFileCounts[remote_path] + 1;
            });

            Mock<ISyncServiceFactory> mockSyncServiceFactory;
            When(OverloadedMethod(mockSyncServiceFactory, CreateCSWebSyncService, std::unique_ptr<ISyncService>(SyncConnectionString, std::unique_ptr<LoginCredentials>)))
                .AlwaysDo([&](const SyncConnectionString&, const std::unique_ptr<LoginCredentials>&) { return std::unique_ptr<ISyncService>(&(mockServer.get())); });

            SyncClient sync_client(myDeviceId, std::unique_ptr<ISyncServiceFactory>(&mockSyncServiceFactory.get()));
            sync_client.SetSyncListener(std::make_unique<SyncLogSyncListener>());

            const std::vector<std::string> filenames =
            {
                "foo1",
                "foo2",
                "fooa",
                "bar",
                "foothree"
            };

            std::string srcDir = Path::Combine(GetTempDirectory(), "syncputtest");
            PortableFunctions::DirectoryDelete(srcDir, true);
            PortableFunctions::PathMakeDirectories(srcDir);

            for( const std::string& filename : filenames )
                FileIO::WriteText(Path::Combine(srcDir, filename), fileContents, false);

            PortableFunctions::PathMakeDirectories(Path::Combine(srcDir, "foodir"));

            auto connectOkResult = sync_client.ConnectCSWeb(hostUrl, std::make_unique<LoginCredentials>(username, password));
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, connectOkResult);

            SyncClient::SyncResult syncResult = sync_client.SyncFile(SyncDirection::Put, Path::Combine(srcDir, "*"), "/");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, syncResult);

            syncResult = sync_client.SyncFile(SyncDirection::Put, Path::Combine(srcDir, "foo*"), "/");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, syncResult);

            syncResult = sync_client.SyncFile(SyncDirection::Put, Path::Combine(srcDir, "foo?"), "/");
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, syncResult);

            Assert::AreEqual(putFileCounts["/" + filenames[0]], 3);
            Assert::AreEqual(putFileCounts["/" + filenames[1]], 3);
            Assert::AreEqual(putFileCounts["/" + filenames[2]], 3);
            Assert::AreEqual(putFileCounts["/" + filenames[3]], 1);
            Assert::AreEqual(putFileCounts["/" + filenames[4]], 2);
            Assert::AreEqual(putFileCounts.size(), size_t(5));

            auto disconnectResult = sync_client.Disconnect();
            Assert::AreEqual(SyncClient::SyncResult::SYNC_OK, disconnectResult);

            PortableFunctions::DirectoryDelete(srcDir, true);
        }
    };
}
