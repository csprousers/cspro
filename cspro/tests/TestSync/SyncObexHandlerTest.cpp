#include "stdafx.h"
#include "CaseTestHelpers.h"
#include "TestRepoBuilder.h"
#include <zToolsO/PortableFunctions.h>
#include <zUtilO/Versioning.h>
#include <zDictO/DDClass.h>
#include <zNetwork/SyncCustomHeaders.h>
#include <zSyncO/SyncObexHandler.h>
#include <fstream>

using namespace fakeit;


namespace SyncUnitTest
{
    TEST_CLASS(SyncObexHandlerTest)
    {
    private:
        const DeviceId serverDeviceId = "serverid";
        const DeviceId clientDeviceId = "clientid";
        const std::string dictionary = "popstan";

    public:
        TEST_METHOD(TestGetDeviceId)
        {
            SyncObexHandler handler(serverDeviceId, std::string(), nullptr);
            std::unique_ptr<IObexResource> resource;
            ObexResponseCode responseCode = handler.onGet(OBEX_SYNC_DATA_MEDIA_TYPE, L"", HeaderList(), resource);
            Assert::AreEqual((int)OBEX_OK, (int)responseCode);
            resource->openForReading();
            std::string responseJson;
            responseJson = std::string(std::istreambuf_iterator<char>(*resource->getIStream()), {});
            ConnectResponse response = Json::FromJson<ConnectResponse>(responseJson);
            Assert::AreEqual(serverDeviceId, response.GetServerDeviceId());
        }


        TEST_METHOD(TestSync)
        {
            std::unique_ptr<const CDataDict> data_dictionary = CreateTestDictionary();
            std::shared_ptr<CaseAccess> case_access = CaseAccess::CreateAndInitializeFullCaseAccess(*data_dictionary);
            TestRepoBuilder serverRepoBuilder(serverDeviceId, data_dictionary.get());
            ISyncableDataRepository* pServerRepo = serverRepoBuilder.GetRepo();

            Mock<ISyncObexEngineAccessor> mockEngineAccessor;
            When(Method(mockEngineAccessor, GetDataRepository)).AlwaysReturn(pServerRepo);

            SyncObexHandler handler(serverDeviceId, std::string(), std::unique_ptr<ISyncObexEngineAccessor>(&mockEngineAccessor.get()));

            // Add a few initial cases to the server
            std::shared_ptr<Case> initServerCase1 = CreateCase(*case_access, "guid1", 1, { "initialserverdata1" });
            std::shared_ptr<Case> initServerCase2 = CreateCase(*case_access, "guid2", 2, { "initialserverdata2" });
            std::shared_ptr<Case> initServerCase3 = CreateCase(*case_access, "guid3", 3, { "initialserverdata3" });
            VectorClock clockServer1;
            clockServer1.increment(serverDeviceId);
            initServerCase1->SetVectorClock(clockServer1);
            initServerCase2->SetVectorClock(clockServer1);
            VectorClock clockServer2;
            clockServer2.increment(serverDeviceId);
            clockServer2.increment(serverDeviceId);
            initServerCase3->SetVectorClock(clockServer2);

            std::vector<std::shared_ptr<Case>> serverCases = { initServerCase1, initServerCase2, initServerCase3 };
            serverRepoBuilder.setInitialRepoCases(serverCases, serverDeviceId);

            // Create cases to put from client
            std::shared_ptr<Case> clientCase1 = CreateCase(*case_access, "guid1", 1, { "newclientdata1" });
            std::shared_ptr<Case> clientCase2 = CreateCase(*case_access, "guid2", 2, { "newclientdata2" });
            std::shared_ptr<Case> clientCase3 = CreateCase(*case_access, "guid3", 3, { "newclientdata3" });
            std::shared_ptr<Case> clientCase4 = CreateCase(*case_access, "guid4", 4, { "newclientdata4" });
            VectorClock clockClient1;
            clockClient1.increment(clientDeviceId);
            clientCase1->SetVectorClock(clockClient1);
            VectorClock clockServer1Client1;
            clockServer1Client1.increment(clientDeviceId);
            clockServer1Client1.increment(serverDeviceId);
            clientCase2->SetVectorClock(clockServer1Client1);
            clientCase3->SetVectorClock(clockServer1);
            clientCase4->SetVectorClock(clockClient1);
            std::vector<std::shared_ptr<Case>> clientCases = { clientCase1, clientCase2, clientCase3, clientCase4 };

            const std::string syncPath = "/dictionaries/" + dictionary + "/syncs";
            HeaderList requestHeaders;
            requestHeaders.Add_UserAgent_CSProSyncClient();
            requestHeaders.Add(SyncCustomHeaders::DEVICE_ID_HEADER, clientDeviceId);
            std::unique_ptr<IObexResource> resource;
            ObexResponseCode responseCode = handler.onGet(OBEX_SYNC_DATA_MEDIA_TYPE, UTF8_TODO::GetCString(syncPath), requestHeaders, resource);
            Assert::AreEqual((int)OBEX_OK, (int)responseCode);
            Assert::AreEqual((int)OBEX_OK, (int)resource->openForReading());
            std::string responseJson = std::string(std::istreambuf_iterator<char>(*resource->getIStream()), {});
            ZLib::Inflate(responseJson);
            HeaderList responseHeaders = resource->getHeaders();
            Assert::IsFalse(responseHeaders.GetValue("ETag").empty(), L"Response missing etag");
            std::string serverRevisionFromGet = responseHeaders.GetValue("ETag");
            resource.reset();

            SyncCaseSerializer sync_case_serializer = SyncCaseSerializer::CreateFromCSProVersion(case_access, responseHeaders);
            std::vector<std::shared_ptr<Case>> responseCases = sync_case_serializer.ParseSyncableCaseData(responseJson);

            // First get, should retrieve all three server cases
            Assert::AreEqual(3, (int)responseCases.size());
            Verify(Method(mockEngineAccessor, GetDataRepository)).Exactly(1);

            std::string putJson = GetSyncableCaseData(case_access, SpanHelpers::CreatePointersSpan(clientCases), SyncCaseSerializer::Version::V2);
            responseCode = handler.onPut(OBEX_SYNC_DATA_MEDIA_TYPE, UTF8_TODO::GetCString(syncPath), requestHeaders, resource);
            Assert::AreEqual((int)OBEX_OK, (int)responseCode);
            resource->openForWriting();
            resource->getOStream()->write(&putJson[0], putJson.size());
            Assert::AreEqual((int)OBEX_OK, (int)resource->openForReading());
            Assert::IsNull(resource->getIStream());
            Assert::AreEqual((int)OBEX_OK, (int)resource->close());
            responseHeaders = resource->getHeaders();
            Assert::IsFalse(responseHeaders.GetValue("ETag").empty(), L"Response missing etag");
            const std::string serverRevisionFromPut = responseHeaders.GetValue("ETag");
            resource.reset();

            // Case 1 will be a conflict and should be overwritten with client data since client always wins conflicts
            serverRepoBuilder.verifyCaseInRepo(*clientCase1);

            // Case 2 is newer on client and should be overwritten with client data
            serverRepoBuilder.verifyCaseInRepo(*clientCase2);

            // Case 3 is newer on server and should NOT be overwritten
            serverRepoBuilder.verifyCaseInRepo(*initServerCase3);

            // Case 4 is only on client and should be written to server
            serverRepoBuilder.verifyCaseInRepo(*clientCase4);

            // There should now be four cases on server
            Assert::AreEqual(4, (int)pServerRepo->GetNumberCases());

            // Sync again with no changes should retrieve no new cases
            requestHeaders.Add(SyncCustomHeaders::IF_REVISION_EXISTS_HEADER, serverRevisionFromGet);
            requestHeaders.Add(SyncCustomHeaders::EXCLUDE_REVISIONS_HEADER, serverRevisionFromPut);
            responseCode = handler.onGet(OBEX_SYNC_DATA_MEDIA_TYPE, UTF8_TODO::GetCString(syncPath), requestHeaders, resource);
            Assert::AreEqual((int)OBEX_OK, (int)responseCode);
            Assert::AreEqual((int)OBEX_OK, (int)resource->openForReading());
            responseJson = std::string(std::istreambuf_iterator<char>(*resource->getIStream()), {});
            ZLib::Inflate(responseJson);
            responseCases = sync_case_serializer.ParseSyncableCaseData(responseJson);
            Assert::AreEqual(0, (int)responseCases.size());

            responseHeaders = resource->getHeaders();
            Assert::IsFalse(responseHeaders.GetValue("ETag").empty(), L"Response missing etag");
            serverRevisionFromGet = responseHeaders.GetValue("ETag");
            resource.reset();

            // Update a case on the server and get and should get updated case
            serverRepoBuilder.updateRepoCase(initServerCase1->GetUuid(), 1, "updatedserverdata1");

            requestHeaders = HeaderList();
            requestHeaders.Add_UserAgent_CSProSyncClient();
            requestHeaders.Add(SyncCustomHeaders::IF_REVISION_EXISTS_HEADER, serverRevisionFromGet);
            requestHeaders.Add(SyncCustomHeaders::EXCLUDE_REVISIONS_HEADER, serverRevisionFromPut);
            requestHeaders.Add(SyncCustomHeaders::DEVICE_ID_HEADER, clientDeviceId);
            responseCode = handler.onGet(OBEX_SYNC_DATA_MEDIA_TYPE, UTF8_TODO::GetCString(syncPath), requestHeaders, resource);
            Assert::AreEqual((int)OBEX_OK, (int)responseCode);
            Assert::AreEqual((int)OBEX_OK, (int)resource->openForReading());
            responseJson = std::string(std::istreambuf_iterator<char>(*resource->getIStream()), {});
            ZLib::Inflate(responseJson);
            responseCases = sync_case_serializer.ParseSyncableCaseData(responseJson);
            responseHeaders = resource->getHeaders();
            Assert::IsFalse(responseHeaders.GetValue("ETag").empty(), L"Response missing etag");
            serverRevisionFromGet = responseHeaders.GetValue("ETag");
            resource.reset();
            Assert::AreEqual(1, (int)responseCases.size());

            // Add new case on client and put
            std::shared_ptr<Case> clientCase5 = CreateCase(*case_access, "guid5", 1, { "newclientdata5" });
            clientCase5->SetVectorClock(clockClient1);
            putJson = GetSyncableCaseData(case_access, { clientCase5.get() }, SyncCaseSerializer::Version::V2);
            requestHeaders = HeaderList();
            requestHeaders.Add_UserAgent_CSProSyncClient();
            requestHeaders.Add(SyncCustomHeaders::IF_REVISION_EXISTS_HEADER, serverRevisionFromPut);
            requestHeaders.Add(SyncCustomHeaders::DEVICE_ID_HEADER, clientDeviceId);
            responseCode = handler.onPut(OBEX_SYNC_DATA_MEDIA_TYPE, UTF8_TODO::GetCString(syncPath), requestHeaders, resource);
            Assert::AreEqual((int)OBEX_OK, (int)responseCode);
            resource->openForWriting();
            resource->getOStream()->write(&putJson[0], putJson.size());
            Assert::AreEqual((int)OBEX_OK, (int)resource->openForReading());
            Assert::IsNull(resource->getIStream());
            Assert::AreEqual((int)OBEX_OK, (int)resource->close());
            responseHeaders = resource->getHeaders();
            Assert::IsFalse(responseHeaders.GetValue("ETag").empty(), L"Response missing etag");
            const std::string serverRevisionFromPut2 = responseHeaders.GetValue("ETag");
            resource.reset();

            // Get again, should get no new cases
            requestHeaders = HeaderList();
            requestHeaders.Add_UserAgent_CSProSyncClient();
            requestHeaders.Add(SyncCustomHeaders::IF_REVISION_EXISTS_HEADER, serverRevisionFromGet);
            requestHeaders.Add(SyncCustomHeaders::EXCLUDE_REVISIONS_HEADER, serverRevisionFromPut + "," + serverRevisionFromPut2);
            requestHeaders.Add(SyncCustomHeaders::DEVICE_ID_HEADER, clientDeviceId);
            responseCode = handler.onGet(OBEX_SYNC_DATA_MEDIA_TYPE, UTF8_TODO::GetCString(syncPath), requestHeaders, resource);
            Assert::AreEqual((int)OBEX_OK, (int)responseCode);
            Assert::AreEqual((int)OBEX_OK, (int)resource->openForReading());
            responseJson = std::string(std::istreambuf_iterator<char>(*resource->getIStream()), {});
            ZLib::Inflate(responseJson);
            responseCases = sync_case_serializer.ParseSyncableCaseData(responseJson);
            responseHeaders = resource->getHeaders();
            Assert::IsFalse(responseHeaders.GetValue("ETag").empty(), L"Response missing etag");
            resource.reset();
            Assert::AreEqual(0, (int)responseCases.size());
        }


        TEST_METHOD(TestSyncServiceHistoryLost)
        {
            std::unique_ptr<const CDataDict> pDictAP = CreateTestDictionary();
            const CDataDict* pDict = pDictAP.get();
            std::shared_ptr<CaseAccess> case_access = CaseAccess::CreateAndInitializeFullCaseAccess(*pDict);

            TestRepoBuilder serverRepoBuilder(serverDeviceId, pDict);
            ISyncableDataRepository* pServerRepo = serverRepoBuilder.GetRepo();

            Mock<ISyncObexEngineAccessor> mockEngineAccessor;
            When(Method(mockEngineAccessor, GetDataRepository)).AlwaysReturn(pServerRepo);

            SyncObexHandler handler(serverDeviceId, std::string(), std::unique_ptr<ISyncObexEngineAccessor>(&mockEngineAccessor.get()));

            // Add a few initial cases to the server
            VectorClock clockServer1Client1;
            clockServer1Client1.increment(clientDeviceId);
            clockServer1Client1.increment(serverDeviceId);
            std::shared_ptr<Case> initServerCase1 = CreateCase(*case_access, "guid1", 1, { "initialserverdata1" });
            initServerCase1->SetVectorClock(clockServer1Client1);
            VectorClock clockServer1;
            clockServer1.increment(serverDeviceId);

            std::shared_ptr<Case> initServerCase2 = CreateCase(*case_access, "guid2", 2, { "initialserverdata2" });
            initServerCase2->SetVectorClock(clockServer1);
            std::vector<std::shared_ptr<Case>> serverCases = { initServerCase1, initServerCase2 };
            serverRepoBuilder.setInitialRepoCases(serverCases, serverDeviceId);

            std::vector<std::shared_ptr<Case>> clientCases;

            // Get cases from server
            const std::string syncPath = "/dictionaries/" + dictionary + "/syncs";
            HeaderList requestHeaders;
            requestHeaders.Add_UserAgent_CSProSyncClient();
            requestHeaders.Add(SyncCustomHeaders::DEVICE_ID_HEADER, clientDeviceId);
            std::unique_ptr<IObexResource> resource;
            ObexResponseCode responseCode = handler.onGet(OBEX_SYNC_DATA_MEDIA_TYPE, UTF8_TODO::GetCString(syncPath), requestHeaders, resource);
            Assert::AreEqual((int)OBEX_OK, (int)responseCode);
            std::string responseJson;
            Assert::AreEqual((int)OBEX_OK, (int)resource->openForReading());
            responseJson = std::string(std::istreambuf_iterator<char>(*resource->getIStream()), {});
            ZLib::Inflate(responseJson);
            HeaderList responseHeaders = resource->getHeaders();
            Assert::IsFalse(responseHeaders.GetValue("ETag").empty(), L"Response missing etag");
            resource.reset();

            SyncCaseSerializer sync_case_serializer = SyncCaseSerializer::CreateFromCSProVersion(case_access, responseHeaders);
            std::vector<std::shared_ptr<Case>> responseCases = sync_case_serializer.ParseSyncableCaseData(responseJson);

            // First get, should retrieve the two server cases
            Assert::AreEqual(2, (int) responseCases.size());

            // Sync again with get without the etag should retrieve same 2 cases again
            responseCode = handler.onGet(OBEX_SYNC_DATA_MEDIA_TYPE, UTF8_TODO::GetCString(syncPath), requestHeaders, resource);
            Assert::AreEqual((int)OBEX_OK, (int)responseCode);
            Assert::AreEqual((int)OBEX_OK, (int)resource->openForReading());
            responseJson = std::string(std::istreambuf_iterator<char>(*resource->getIStream()), {});
            ZLib::Inflate(responseJson);
            responseHeaders = resource->getHeaders();
            Assert::IsFalse(responseHeaders.GetValue("ETag").empty(), L"Response missing etag");
            responseCases = sync_case_serializer.ParseSyncableCaseData(responseJson);

            Assert::AreEqual(2, (int) responseCases.size());
            resource.reset();

            // Delete server history and use etag, should get precondition failed
            serverRepoBuilder.ResetRepo(serverDeviceId, pDict);
            pServerRepo = serverRepoBuilder.GetRepo();
            When(Method(mockEngineAccessor, GetDataRepository)).AlwaysReturn(pServerRepo);

            requestHeaders.Add(SyncCustomHeaders::IF_REVISION_EXISTS_HEADER, responseHeaders.GetValue("ETag"));
            responseCode = handler.onGet(OBEX_SYNC_DATA_MEDIA_TYPE, UTF8_TODO::GetCString(syncPath), requestHeaders, resource);
            Assert::AreEqual((int)OBEX_OK, (int)responseCode);
            Assert::AreEqual((int)OBEX_PRECONDITION_FAILED, (int)resource->openForReading());
            resource.reset();

        }


        TEST_METHOD(TestSyncServiceHistoryLostPut)
        {
            std::unique_ptr<const CDataDict> pDictAP = CreateTestDictionary();
            const CDataDict* pDict = pDictAP.get();
            std::shared_ptr<CaseAccess> case_access = CaseAccess::CreateAndInitializeFullCaseAccess(*pDict);

            TestRepoBuilder serverRepoBuilder(serverDeviceId, pDict);
            ISyncableDataRepository* pServerRepo = serverRepoBuilder.GetRepo();

            Mock<ISyncObexEngineAccessor> mockEngineAccessor;
            When(Method(mockEngineAccessor, GetDataRepository)).AlwaysReturn(pServerRepo);

            SyncObexHandler handler(serverDeviceId, std::string(), std::unique_ptr<ISyncObexEngineAccessor>(&mockEngineAccessor.get()));

            // Add a few initial cases to the client
            VectorClock clockClient1;
            clockClient1.increment(clientDeviceId);
            std::shared_ptr<Case> clientCase1 = CreateCase(*case_access, "guid1", 1, { "initialclientdata1" });
            clientCase1->SetVectorClock(clockClient1);
            std::shared_ptr<Case> clientCase2 = CreateCase(*case_access, "guid2", 2, { "initialclientdata2" });
            clientCase2->SetVectorClock(clockClient1);

            // Put cases to server
            const std::string syncPath = "/dictionaries/" + dictionary + "/syncs";
            HeaderList requestHeaders;
            requestHeaders.Add_UserAgent_CSProSyncClient();
            requestHeaders.Add(SyncCustomHeaders::DEVICE_ID_HEADER, clientDeviceId);
            std::string putJson = GetSyncableCaseData(case_access, { clientCase1.get(), clientCase2.get() }, SyncCaseSerializer::Version::V2);
            std::unique_ptr<IObexResource> resource;
            ObexResponseCode responseCode = handler.onPut(OBEX_SYNC_DATA_MEDIA_TYPE, UTF8_TODO::GetCString(syncPath), requestHeaders, resource);
            Assert::AreEqual((int)OBEX_OK, (int)responseCode);
            resource->openForWriting();
            resource->getOStream()->write(&putJson[0], putJson.size());
            Assert::AreEqual((int)OBEX_OK, (int)resource->openForReading());
            Assert::IsNull(resource->getIStream());
            Assert::AreEqual((int)OBEX_OK, (int)resource->close());
            HeaderList responseHeaders = resource->getHeaders();
            Assert::IsFalse(responseHeaders.GetValue("ETag").empty(), L"Response missing etag");
            const std::string serverRevisionFromPut = responseHeaders.GetValue("ETag");
            resource.reset();

            // Delete server history and use etag, should get precondition failed
            serverRepoBuilder.ResetRepo(serverDeviceId, pDict);
            pServerRepo = serverRepoBuilder.GetRepo();
            When(Method(mockEngineAccessor, GetDataRepository)).AlwaysReturn(pServerRepo);

            requestHeaders.Add(SyncCustomHeaders::IF_REVISION_EXISTS_HEADER, serverRevisionFromPut);
            responseCode = handler.onPut(OBEX_SYNC_DATA_MEDIA_TYPE, UTF8_TODO::GetCString(syncPath), requestHeaders, resource);
            Assert::AreEqual((int)OBEX_OK, (int)responseCode);
            Assert::AreEqual((int)OBEX_PRECONDITION_FAILED, (int)resource->openForWriting());
            resource.reset();

            // Add a case on server from a different client
            std::shared_ptr<Case> clientCase3 = CreateCase(*case_access, "guid3", 3, { "13newclientdata3" });
            clientCase3->SetVectorClock(clockClient1);
            putJson = GetSyncableCaseData(case_access, { clientCase3.get() }, SyncCaseSerializer::Version::V2);
            HeaderList requestHeadersClient2;
            requestHeadersClient2.Add(SyncCustomHeaders::DEVICE_ID_HEADER, ": client2");
            responseCode = handler.onPut(OBEX_SYNC_DATA_MEDIA_TYPE, UTF8_TODO::GetCString(syncPath), requestHeadersClient2, resource);
            Assert::AreEqual((int)OBEX_OK, (int)responseCode);
            resource->openForWriting();
            resource->getOStream()->write(&putJson[0], putJson.size());
            Assert::AreEqual((int)OBEX_OK, (int)resource->openForReading());
            Assert::IsNull(resource->getIStream());
            Assert::AreEqual((int)OBEX_OK, (int)resource->close());
            responseHeaders = resource->getHeaders();
            Assert::IsFalse(responseHeaders.GetValue("ETag").empty(), L"Response missing etag");
            const std::string serverRevisionFromPut2 = responseHeaders.GetValue("ETag");
            resource.reset();

            // Sync again from client1 - should still fail on precondition
            responseCode = handler.onPut(OBEX_SYNC_DATA_MEDIA_TYPE, UTF8_TODO::GetCString(syncPath), requestHeaders, resource);
            Assert::AreEqual((int)OBEX_OK, (int)responseCode);
            Assert::AreEqual((int)OBEX_PRECONDITION_FAILED, (int)resource->openForWriting());
            resource.reset();

        }


        TEST_METHOD(TestSyncResumableGet)
        {
            std::unique_ptr<const CDataDict> pDictAP = CreateTestDictionary();
            const CDataDict* pDict = pDictAP.get();
            std::shared_ptr<CaseAccess> case_access = CaseAccess::CreateAndInitializeFullCaseAccess(*pDict);
            TestRepoBuilder serverRepoBuilder(serverDeviceId, pDict);
            ISyncableDataRepository* pServerRepo = serverRepoBuilder.GetRepo();

            Mock<ISyncObexEngineAccessor> mockEngineAccessor;
            When(Method(mockEngineAccessor, GetDataRepository)).AlwaysReturn(pServerRepo);

            SyncObexHandler handler(serverDeviceId, std::string(), std::unique_ptr<ISyncObexEngineAccessor>(&mockEngineAccessor.get()));

            const std::string syncPath = "/dictionaries/" + dictionary + "/syncs";
            std::unique_ptr<IObexResource> resource;

            std::vector<std::shared_ptr<Case>> serverCases;
            std::vector<std::shared_ptr<Case>> clientCases;

            // Start with 30 cases on server
            VectorClock clockServer;
            clockServer.increment(serverDeviceId);
            for (int i = 0; i < 30; ++i) {
                std::string uuid = FormatText("guid%02d", i);
                std::shared_ptr<Case> initServerCase = CreateCase(*case_access, uuid, i % 10, { "initialserverdata" });
                initServerCase->SetVectorClock(clockServer);
                serverCases.emplace_back(initServerCase);
            }
            serverRepoBuilder.setInitialRepoCases(serverCases, serverDeviceId);

            // Get 10 cases
            HeaderList requestHeaders;
            requestHeaders.Add_UserAgent_CSProSyncClient();
            requestHeaders.Add(SyncCustomHeaders::DEVICE_ID_HEADER, clientDeviceId);
            requestHeaders.Add(SyncCustomHeaders::RANGE_COUNT_HEADER, "10");
            ObexResponseCode responseCode = handler.onGet(OBEX_SYNC_DATA_MEDIA_TYPE, UTF8_TODO::GetCString(syncPath), requestHeaders, resource);
            Assert::AreEqual((int) OBEX_OK, (int) responseCode);
            Assert::AreEqual((int) OBEX_PARTIAL_CONTENT, (int) resource->openForReading());
            std::string responseJson = std::string(std::istreambuf_iterator<char>(*resource->getIStream()), {});
            ZLib::Inflate(responseJson);
            HeaderList responseHeaders = resource->getHeaders();
            Assert::IsFalse(responseHeaders.GetValue("ETag").empty(), L"Response missing etag");
            resource.reset();

            SyncCaseSerializer sync_case_serializer = SyncCaseSerializer::CreateFromCSProVersion(case_access, responseHeaders);
            std::vector<std::shared_ptr<Case>> responseCases = sync_case_serializer.ParseSyncableCaseData(responseJson);
            Assert::AreEqual(size_t(10), responseCases.size());
            MergeCaseList(clientCases, responseCases);

            // Get next 10 cases
            requestHeaders = HeaderList();
            requestHeaders.Add_UserAgent_CSProSyncClient();
            requestHeaders.Add(SyncCustomHeaders::DEVICE_ID_HEADER, clientDeviceId);
            requestHeaders.Add(SyncCustomHeaders::RANGE_COUNT_HEADER, "10");
            requestHeaders.Add(SyncCustomHeaders::START_AFTER_HEADER, responseCases.back()->GetUuid());
            requestHeaders.Add(SyncCustomHeaders::IF_REVISION_EXISTS_HEADER, responseHeaders.GetValue("ETag"));
            responseCode = handler.onGet(OBEX_SYNC_DATA_MEDIA_TYPE, UTF8_TODO::GetCString(syncPath), requestHeaders, resource);
            Assert::AreEqual((int) OBEX_OK, (int) responseCode);
            Assert::AreEqual((int) OBEX_PARTIAL_CONTENT, (int) resource->openForReading());
            responseJson = std::string(std::istreambuf_iterator<char>(*resource->getIStream()), {});
            ZLib::Inflate(responseJson);
            responseHeaders = resource->getHeaders();
            Assert::IsFalse(responseHeaders.GetValue("ETag").empty(), L"Response missing etag");
            resource.reset();
            responseCases = sync_case_serializer.ParseSyncableCaseData(responseJson);
            Assert::AreEqual(size_t(10), responseCases.size());
            MergeCaseList(clientCases, responseCases);

            // Add a new case on server and modify existing cases
            serverCases.emplace_back(serverRepoBuilder.addRepoCase("aaaa", 1, { "addeddata" }, clockServer));
            serverCases[10] = serverRepoBuilder.updateRepoCase(serverCases[10]->GetUuid(), 10 % 10, "updateddata");
            serverCases[25] = serverRepoBuilder.updateRepoCase(serverCases[25]->GetUuid(), 25 % 10, "updateddata");

            // Get next 10 cases
            requestHeaders = HeaderList();
            requestHeaders.Add_UserAgent_CSProSyncClient();
            requestHeaders.Add(SyncCustomHeaders::DEVICE_ID_HEADER, clientDeviceId);
            requestHeaders.Add(SyncCustomHeaders::RANGE_COUNT_HEADER, "10");
            requestHeaders.Add(SyncCustomHeaders::START_AFTER_HEADER, responseCases.back()->GetUuid());
            requestHeaders.Add(SyncCustomHeaders::IF_REVISION_EXISTS_HEADER, responseHeaders.GetValue("ETag"));
            responseCode = handler.onGet(OBEX_SYNC_DATA_MEDIA_TYPE, UTF8_TODO::GetCString(syncPath), requestHeaders, resource);
            Assert::AreEqual((int) OBEX_OK, (int) responseCode);
            Assert::AreEqual((int) OBEX_PARTIAL_CONTENT, (int) resource->openForReading());
            responseJson = std::string(std::istreambuf_iterator<char>(*resource->getIStream()), {});
            ZLib::Inflate(responseJson);
            responseHeaders = resource->getHeaders();
            Assert::IsFalse(responseHeaders.GetValue("ETag").empty(), L"Response missing etag");
            resource.reset();
            responseCases = sync_case_serializer.ParseSyncableCaseData(responseJson);
            Assert::AreEqual(size_t(10), responseCases.size());
            MergeCaseList(clientCases, responseCases);

            // Get remaining 2 cases
            requestHeaders = HeaderList();
            requestHeaders.Add_UserAgent_CSProSyncClient();
            requestHeaders.Add(SyncCustomHeaders::DEVICE_ID_HEADER, clientDeviceId);
            requestHeaders.Add(SyncCustomHeaders::RANGE_COUNT_HEADER, "10");
            requestHeaders.Add(SyncCustomHeaders::START_AFTER_HEADER, responseCases.back()->GetUuid());
            requestHeaders.Add(SyncCustomHeaders::IF_REVISION_EXISTS_HEADER, responseHeaders.GetValue("ETag"));
            responseCode = handler.onGet(OBEX_SYNC_DATA_MEDIA_TYPE, UTF8_TODO::GetCString(syncPath), requestHeaders, resource);
            Assert::AreEqual((int) OBEX_OK, (int) responseCode);
            Assert::AreEqual((int) OBEX_OK, (int) resource->openForReading());
            responseJson = std::string(std::istreambuf_iterator<char>(*resource->getIStream()), {});
            ZLib::Inflate(responseJson);
            responseHeaders = resource->getHeaders();
            Assert::IsFalse(responseHeaders.GetValue("ETag").empty(), L"Response missing etag");
            resource.reset();
            responseCases = sync_case_serializer.ParseSyncableCaseData(responseJson);

            Assert::AreEqual(size_t(2), responseCases.size());
            MergeCaseList(clientCases, responseCases);

            CompareCases(SpanHelpers::CreatePointersSpan(serverCases),
                         SpanHelpers::CreatePointersSpan(clientCases),
                         CompareCasesType::CountsAndBinaryData);
        }


        TEST_METHOD(TestGetFile)
        {
            const std::string rootPath = Path::Combine(GetTempDirectory(), "syncobexhandlertest/");
            PortableFunctions::DirectoryDelete(rootPath, true);
            PortableFunctions::PathMakeDirectories(rootPath);

            const std::string fileContents("SOME DATA");
            const std::string testFile = "test-get.txt";
            FileIO::WriteText(rootPath + testFile, fileContents, false);

            SyncObexHandler handler(serverDeviceId, rootPath, nullptr);

            std::unique_ptr<IObexResource> resource;
            ObexResponseCode responseCode = handler.onGet(OBEX_BINARY_FILE_MEDIA_TYPE, UTF8_TODO::GetCString(testFile), HeaderList(), resource);
            Assert::AreEqual((int)OBEX_OK, (int)responseCode);
            Assert::AreEqual((int)OBEX_IS_LAST_FILE_CHUNK, (int)resource->openForReading());

            std::string response(std::istreambuf_iterator<char>(*resource->getIStream()), {});
            ZLib::Inflate(response);
            Assert::AreEqual(fileContents, response);
            resource.reset();

            // Etag should return not-modified
            HeaderList requestHeaders;
            requestHeaders.Add_UserAgent_CSProSyncClient();
            requestHeaders.Add_IfNoneMatch("662411C1698ECC13DD07AEE13439EADC");
            responseCode = handler.onGet(OBEX_BINARY_FILE_MEDIA_TYPE, UTF8_TODO::GetCString(testFile), requestHeaders, resource);
            Assert::AreEqual((int)OBEX_NOT_MODIFIED, (int)responseCode);

            // Non-matching Etag should download
            requestHeaders = HeaderList();
            requestHeaders.Add_UserAgent_CSProSyncClient();
            requestHeaders.Add_IfNoneMatch("1234567890");
            responseCode = handler.onGet(OBEX_BINARY_FILE_MEDIA_TYPE, UTF8_TODO::GetCString(testFile), requestHeaders, resource);
            Assert::AreEqual((int)OBEX_OK, (int)responseCode);
            resource.reset();

            // Non-existent file should fail
            responseCode = handler.onGet(OBEX_BINARY_FILE_MEDIA_TYPE, "file-that-doesnt-exist.txt", HeaderList(), resource);
            Assert::AreEqual((int)OBEX_NOT_FOUND, (int)responseCode);

            PortableFunctions::DirectoryDelete(rootPath);
        }


        TEST_METHOD(TestPutFile)
        {
            const std::string rootPath = Path::Combine(GetTempDirectory(), "syncobexhandlertest/");
            PortableFunctions::DirectoryDelete(rootPath, true);
            PortableFunctions::PathMakeDirectories(rootPath);

            const std::string fileContents("SOME DATA");
            const std::string testFile = "test-put.txt";

            SyncObexHandler handler(serverDeviceId, rootPath, nullptr);

            std::unique_ptr<IObexResource> resource;
            ObexResponseCode responseCode = handler.onPut(OBEX_BINARY_FILE_MEDIA_TYPE, UTF8_TODO::GetCString(testFile), HeaderList(), resource);
            Assert::AreEqual((int)OBEX_OK, (int)responseCode);
            Assert::AreEqual((int)OBEX_OK, (int)resource->openForWriting());

            resource->getOStream()->write(&fileContents[0], fileContents.size());
            Assert::IsFalse(resource->getOStream()->fail());
            Assert::AreEqual((int) OBEX_OK, (int) resource->close());
            resource.reset();

            Assert::AreEqual(fileContents, FileIO::ReadText(rootPath + testFile));

            PortableFunctions::DirectoryDelete(rootPath);
        }


        TEST_METHOD(TestGetDirectoryListing)
        {
            const std::string rootPath = Path::Combine(GetTempDirectory(), "syncobexhandlertest/");
            PortableFunctions::DirectoryDelete(rootPath, true);
            PortableFunctions::PathMakeDirectories(rootPath);

            const std::string fileContents("SOME DATA");
            const std::string testFile1 = "test1.txt";
            const std::string testFile2 = "test2.txt";
            FileIO::WriteText(rootPath + testFile1, fileContents, false);
            FileIO::WriteText(rootPath + testFile2, fileContents, false);

            const std::string testDir = "test3-dir";
            PortableFunctions::PathMakeDirectories(rootPath + testDir);

            SyncObexHandler handler(serverDeviceId, rootPath, nullptr);

            std::unique_ptr<IObexResource> resource;
            ObexResponseCode responseCode = handler.onGet(OBEX_DIRECTORY_LISTING_MEDIA_TYPE, ".", HeaderList(), resource);
            Assert::AreEqual((int)OBEX_OK, (int)responseCode);
            Assert::AreEqual((int)OBEX_OK, (int)resource->openForReading());

            std::string responseJson;
            responseJson = std::string(std::istreambuf_iterator<char>(*resource->getIStream()), {});
            resource.reset();

            const std::vector<FileInfo> directory_listing = Json::Parse(responseJson).GetArray().GetVector<FileInfo>();
            Assert::IsFalse(directory_listing.empty());

            Assert::AreEqual(testFile1, directory_listing[0].GetName());
            Assert::AreEqual("/.", directory_listing[0].GetDirectory().c_str());
            Assert::AreEqual("662411c1698ecc13dd07aee13439eadc", directory_listing[0].GetMd5().c_str());
            Assert::AreEqual((int) FileInfo::FileType::File, (int) directory_listing[0].GetType());
            Assert::AreEqual((int) fileContents.size(), (int) directory_listing[0].GetSize());
            Assert::AreEqual(testFile2, directory_listing[1].GetName());
            Assert::AreEqual("/.", directory_listing[1].GetDirectory().c_str());
            Assert::AreEqual("662411c1698ecc13dd07aee13439eadc", directory_listing[1].GetMd5().c_str());
            Assert::AreEqual((int)FileInfo::FileType::File, (int)directory_listing[1].GetType());
            Assert::AreEqual((int)fileContents.size(), (int) directory_listing[1].GetSize());
            Assert::AreEqual(testDir, directory_listing[2].GetName());
            Assert::AreEqual((int)FileInfo::FileType::Directory, (int)directory_listing[2].GetType());

            // Non-existent directory should fail
            responseCode = handler.onGet(OBEX_DIRECTORY_LISTING_MEDIA_TYPE, "directory-that-doesnt-exist.txt", HeaderList(), resource);
            Assert::AreEqual((int)OBEX_NOT_FOUND, (int)responseCode);

            PortableFunctions::DirectoryDelete(rootPath);
        }
    };
}
