#include "stdafx.h"

#include "CppUnitTest.h"
#include <zSyncO/SyncObexHandler.h>
#include <zSyncO/JsonConverter.h>
#include <zSyncO/SyncRequest.h>
#include <zSyncO/ConnectResponse.h>
#include <zSyncO/IDataRepositoryRetriever.h>
#include <fstream>
#include "TestRepoBuilder.h"
#include <zDictO/DDClass.h>
#include <zSyncO/SyncCustomHeaders.h>
#include <zZipo/ZipUtility.h>
#include "CaseTestHelpers.h"
#include <zToolsO/PortableFunctions.h>
#include <zUtilO/Versioning.h>

namespace Microsoft {
    namespace VisualStudio {
        namespace CppUnitTestFramework
        {
            // ToString specialization is required for all types used in the AssertTrue macro
            template<> inline std::wstring ToString<CString>(const CString& t) { return std::wstring(t); }
        }
    }
}

namespace {
    JsonConverter jsonConverter;
}

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace fakeit;

namespace SyncUnitTest
{
    TEST_CLASS(SyncObexHandlerTest)
    {
    private:
        const DeviceId serverDeviceId = L"serverid";
        const DeviceId clientDeviceId = L"clientid";
        const CString dictionary = L"popstan";

    private:

        void createTestFile(const wchar_t* path, const char* content)
        {
            std::wofstream s(path, std::ios::binary);
            s << content;
            s.close();
        }

    public:
        TEST_METHOD(TestGetDeviceId)
        {
            SyncObexHandler handler(serverDeviceId, nullptr, CString(), nullptr);
            std::unique_ptr<IObexResource> resource;
            ObexResponseCode responseCode = handler.onGet(OBEX_SYNC_DATA_MEDIA_TYPE, L"", HeaderList(), resource);
            Assert::AreEqual((int)OBEX_OK, (int)responseCode);
            resource->openForReading();
            std::string responseJson;
            responseJson = std::string(std::istreambuf_iterator<char>(*resource->getIStream()), {});
            std::unique_ptr<ConnectResponse> response(jsonConverter.connectResponseFromJson(responseJson));
            Assert::AreEqual(serverDeviceId, response->getServerDeviceId());
        }

        TEST_METHOD(TestSync)
        {
            std::unique_ptr<const CDataDict> data_dictionary = createTestDict();
            std::shared_ptr<CaseAccess> case_access = CaseAccess::CreateAndInitializeFullCaseAccess(*data_dictionary);
            TestRepoBuilder serverRepoBuilder(serverDeviceId, data_dictionary.get());
            ISyncableDataRepository* pServerRepo = serverRepoBuilder.GetRepo();

            Mock<IDataRepositoryRetriever> mockRepoRetriever;
            When(Method(mockRepoRetriever, get)).AlwaysReturn(pServerRepo);

            SyncObexHandler handler(serverDeviceId, &mockRepoRetriever.get(), CString(), nullptr);

            // Add a few initial cases to the server
            auto initServerCase1 = makeCase(*case_access, L"guid1", 1, { L"initialserverdata1" });
            auto initServerCase2 = makeCase(*case_access, L"guid2", 2, { L"initialserverdata2" });
            auto initServerCase3 = makeCase(*case_access, L"guid3", 3, { L"initialserverdata3" });
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
            auto clientCase1 = makeCase(*case_access, L"guid1", 1, { L"newclientdata1" });
            auto clientCase2 = makeCase(*case_access, L"guid2", 2, { L"newclientdata2" });
            auto clientCase3 = makeCase(*case_access, L"guid3", 3, { L"newclientdata3" });
            auto clientCase4 = makeCase(*case_access, L"guid4", 4, { L"newclientdata4" });
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

            CString syncPath = L"/dictionaries/" + dictionary + L"/syncs";
            HeaderList requestHeaders;
            CString userAgentHeader = CString("User-Agent: CSPro sync client/") + Versioning::GetVersionDetailedString();
            requestHeaders.push_back(userAgentHeader);
            requestHeaders.push_back(SyncCustomHeaders::DEVICE_ID_HEADER + _T(": ") + clientDeviceId);
            std::unique_ptr<IObexResource> resource;
            ObexResponseCode responseCode = handler.onGet(OBEX_SYNC_DATA_MEDIA_TYPE, syncPath, requestHeaders, resource);
            Assert::AreEqual((int)OBEX_OK, (int)responseCode);
            Assert::AreEqual((int)OBEX_OK, (int)resource->openForReading());
            std::string responseJson = std::string(std::istreambuf_iterator<char>(*resource->getIStream()), {});
            ZipUtility::decompression(responseJson);
            HeaderList responseHeaders = resource->getHeaders();
            Assert::IsFalse(responseHeaders.value(_T("ETag")).IsEmpty(), L"Response missing etag");
            CString serverRevisionFromGet = responseHeaders.value(_T("ETag"));
            resource.reset();
            auto responseCases = getCaseListFromJson(jsonConverter, responseJson, *case_access);


            // First get, should retrieve all three server cases
            Assert::AreEqual(3, (int)responseCases.size());
            Verify(Method(mockRepoRetriever, get)).Exactly(1);

            std::string putJson = convertCasesToJSON(jsonConverter, clientCases);
            responseCode = handler.onPut(OBEX_SYNC_DATA_MEDIA_TYPE, syncPath, requestHeaders, resource);
            Assert::AreEqual((int)OBEX_OK, (int)responseCode);
            resource->openForWriting();
            resource->getOStream()->write(&putJson[0], putJson.size());
            Assert::AreEqual((int)OBEX_OK, (int)resource->openForReading());
            Assert::IsNull(resource->getIStream());
            Assert::AreEqual((int)OBEX_OK, (int)resource->close());
            responseHeaders = resource->getHeaders();
            Assert::IsFalse(responseHeaders.value(_T("ETag")).IsEmpty(), L"Response missing etag");
            CString serverRevisionFromPut = responseHeaders.value(_T("ETag"));
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
            requestHeaders.push_back(SyncCustomHeaders::IF_REVISION_EXISTS_HEADER + _T(':') + serverRevisionFromGet);
            requestHeaders.push_back(SyncCustomHeaders::EXCLUDE_REVISIONS_HEADER + _T(": ") + serverRevisionFromPut);
            responseCode = handler.onGet(OBEX_SYNC_DATA_MEDIA_TYPE, syncPath, requestHeaders, resource);
            Assert::AreEqual((int)OBEX_OK, (int)responseCode);
            Assert::AreEqual((int)OBEX_OK, (int)resource->openForReading());
            responseJson = std::string(std::istreambuf_iterator<char>(*resource->getIStream()), {});
            ZipUtility::decompression(responseJson);
            responseCases.clear();
            responseCases = getCaseListFromJson(jsonConverter, responseJson, *case_access);
            Assert::AreEqual(0, (int)responseCases.size());

            responseHeaders = resource->getHeaders();
            Assert::IsFalse(responseHeaders.value(_T("ETag")).IsEmpty(), L"Response missing etag");
            serverRevisionFromGet = responseHeaders.value(_T("ETag"));
            resource.reset();

            // Update a case on the server and get and should get updated case
            serverRepoBuilder.updateRepoCase(initServerCase1->GetUuid(), 1, L"updatedserverdata1");

            requestHeaders = HeaderList();
            requestHeaders.push_back(userAgentHeader);
            requestHeaders.push_back(SyncCustomHeaders::IF_REVISION_EXISTS_HEADER + _T(':') + serverRevisionFromGet);
            requestHeaders.push_back(SyncCustomHeaders::EXCLUDE_REVISIONS_HEADER + _T(": ") + serverRevisionFromPut);
            requestHeaders.push_back(SyncCustomHeaders::DEVICE_ID_HEADER + _T(": ") + clientDeviceId);
            responseCode = handler.onGet(OBEX_SYNC_DATA_MEDIA_TYPE, syncPath, requestHeaders, resource);
            Assert::AreEqual((int)OBEX_OK, (int)responseCode);
            Assert::AreEqual((int)OBEX_OK, (int)resource->openForReading());
            responseJson = std::string(std::istreambuf_iterator<char>(*resource->getIStream()), {});
            ZipUtility::decompression(responseJson);
            responseCases.clear();
            responseCases = getCaseListFromJson(jsonConverter, responseJson, *case_access);
            responseHeaders = resource->getHeaders();
            Assert::IsFalse(responseHeaders.value(_T("ETag")).IsEmpty(), L"Response missing etag");
            serverRevisionFromGet = responseHeaders.value(_T("ETag"));
            resource.reset();
            Assert::AreEqual(1, (int)responseCases.size());

            // Add new case on client and put
            auto clientCase5 = makeCase(*case_access, L"guid5", 1, { L"newclientdata5" });
            clientCase5->SetVectorClock(clockClient1);
            putJson = convertCasesToJSON(jsonConverter, { clientCase5 });
            requestHeaders = HeaderList();
            requestHeaders.push_back(userAgentHeader);
            requestHeaders.push_back(SyncCustomHeaders::IF_REVISION_EXISTS_HEADER + _T(':') + serverRevisionFromPut);
            requestHeaders.push_back(SyncCustomHeaders::DEVICE_ID_HEADER + _T(": ") + clientDeviceId);
            responseCode = handler.onPut(OBEX_SYNC_DATA_MEDIA_TYPE, syncPath, requestHeaders, resource);
            Assert::AreEqual((int)OBEX_OK, (int)responseCode);
            resource->openForWriting();
            resource->getOStream()->write(&putJson[0], putJson.size());
            Assert::AreEqual((int)OBEX_OK, (int)resource->openForReading());
            Assert::IsNull(resource->getIStream());
            Assert::AreEqual((int)OBEX_OK, (int)resource->close());
            responseHeaders = resource->getHeaders();
            Assert::IsFalse(responseHeaders.value(_T("ETag")).IsEmpty(), L"Response missing etag");
            CString serverRevisionFromPut2 = responseHeaders.value(_T("ETag"));
            resource.reset();

            // Get again, should get no new cases
            requestHeaders = HeaderList();
            requestHeaders.push_back(userAgentHeader);
            requestHeaders.push_back(SyncCustomHeaders::IF_REVISION_EXISTS_HEADER + _T(':') + serverRevisionFromGet);
            requestHeaders.push_back(SyncCustomHeaders::EXCLUDE_REVISIONS_HEADER + _T(": ") + serverRevisionFromPut + _T(",") + serverRevisionFromPut2);
            requestHeaders.push_back(SyncCustomHeaders::DEVICE_ID_HEADER + _T(": ") + clientDeviceId);
            responseCode = handler.onGet(OBEX_SYNC_DATA_MEDIA_TYPE, syncPath, requestHeaders, resource);
            Assert::AreEqual((int)OBEX_OK, (int)responseCode);
            Assert::AreEqual((int)OBEX_OK, (int)resource->openForReading());
            responseJson = std::string(std::istreambuf_iterator<char>(*resource->getIStream()), {});
            ZipUtility::decompression(responseJson);
            responseCases.clear();
            responseCases = getCaseListFromJson(jsonConverter, responseJson, *case_access);
            responseHeaders = resource->getHeaders();
            Assert::IsFalse(responseHeaders.value(_T("ETag")).IsEmpty(), L"Response missing etag");
            resource.reset();
            Assert::AreEqual(0, (int)responseCases.size());
        }

        TEST_METHOD(TestSyncServerHistoryLost)
        {
            std::unique_ptr<const CDataDict> pDictAP = createTestDict();
            const CDataDict* pDict = pDictAP.get();
            std::shared_ptr<CaseAccess> case_access = CaseAccess::CreateAndInitializeFullCaseAccess(*pDict);

            TestRepoBuilder serverRepoBuilder(serverDeviceId, pDict);
            ISyncableDataRepository* pServerRepo = serverRepoBuilder.GetRepo();

            Mock<IDataRepositoryRetriever> mockRepoRetriever;
            When(Method(mockRepoRetriever, get)).AlwaysReturn(pServerRepo);

            SyncObexHandler handler(serverDeviceId, &mockRepoRetriever.get(), CString(), nullptr);

            // Add a few initial cases to the server
            VectorClock clockServer1Client1;
            clockServer1Client1.increment(clientDeviceId);
            clockServer1Client1.increment(serverDeviceId);
            auto initServerCase1 = makeCase(*case_access, L"guid1", 1, { L"initialserverdata1" });
            initServerCase1->SetVectorClock(clockServer1Client1);
            VectorClock clockServer1;
            clockServer1.increment(serverDeviceId);

            auto initServerCase2 = makeCase(*case_access, L"guid2", 2, { L"initialserverdata2" });
            initServerCase2->SetVectorClock(clockServer1);
            std::vector<std::shared_ptr<Case>> serverCases = { initServerCase1, initServerCase2 };
            serverRepoBuilder.setInitialRepoCases(serverCases, serverDeviceId);

            std::vector<std::shared_ptr<Case>> clientCases;

            // Get cases from server
            CString syncPath = L"/dictionaries/" + dictionary + L"/syncs";
            HeaderList requestHeaders;
            CString userAgentHeader = CString("User-Agent: CSPro sync client/") + Versioning::GetVersionDetailedString();
            requestHeaders.push_back(userAgentHeader);
            requestHeaders.push_back(SyncCustomHeaders::DEVICE_ID_HEADER + _T(": ") + clientDeviceId);
            std::unique_ptr<IObexResource> resource;
            ObexResponseCode responseCode = handler.onGet(OBEX_SYNC_DATA_MEDIA_TYPE, syncPath, requestHeaders, resource);
            Assert::AreEqual((int)OBEX_OK, (int)responseCode);
            std::string responseJson;
            Assert::AreEqual((int)OBEX_OK, (int)resource->openForReading());
            responseJson = std::string(std::istreambuf_iterator<char>(*resource->getIStream()), {});
            ZipUtility::decompression(responseJson);
            HeaderList responseHeaders = resource->getHeaders();
            Assert::IsFalse(responseHeaders.value(_T("ETag")).IsEmpty(), L"Response missing etag");
            resource.reset();

            auto responseCases = getCaseListFromJson(jsonConverter, responseJson, *case_access);

            // First get, should retrieve the two server cases
            Assert::AreEqual(2, (int) responseCases.size());

            // Sync again with get without the etag should retrieve same 2 cases again
            responseCode = handler.onGet(OBEX_SYNC_DATA_MEDIA_TYPE, syncPath, requestHeaders, resource);
            Assert::AreEqual((int)OBEX_OK, (int)responseCode);
            Assert::AreEqual((int)OBEX_OK, (int)resource->openForReading());
            responseJson = std::string(std::istreambuf_iterator<char>(*resource->getIStream()), {});
            ZipUtility::decompression(responseJson);
            responseHeaders = resource->getHeaders();
            Assert::IsFalse(responseHeaders.value(_T("ETag")).IsEmpty(), L"Response missing etag");
            responseCases = getCaseListFromJson(jsonConverter, responseJson, *case_access);

            Assert::AreEqual(2, (int) responseCases.size());
            resource.reset();

            // Delete server history and use etag, should get precondition failed
            serverRepoBuilder.ResetRepo(serverDeviceId, pDict);
            pServerRepo = serverRepoBuilder.GetRepo();
            When(Method(mockRepoRetriever, get)).AlwaysReturn(pServerRepo);

            requestHeaders.push_back(SyncCustomHeaders::IF_REVISION_EXISTS_HEADER + _T(':') + responseHeaders.value(_T("ETag")));
            responseCode = handler.onGet(OBEX_SYNC_DATA_MEDIA_TYPE, syncPath, requestHeaders, resource);
            Assert::AreEqual((int)OBEX_OK, (int)responseCode);
            Assert::AreEqual((int)OBEX_PRECONDITION_FAILED, (int)resource->openForReading());
            resource.reset();

        }


        TEST_METHOD(TestSyncServerHistoryLostPut)
        {
            std::unique_ptr<const CDataDict> pDictAP = createTestDict();
            const CDataDict* pDict = pDictAP.get();
            std::shared_ptr<CaseAccess> case_access = CaseAccess::CreateAndInitializeFullCaseAccess(*pDict);

            TestRepoBuilder serverRepoBuilder(serverDeviceId, pDict);
            ISyncableDataRepository* pServerRepo = serverRepoBuilder.GetRepo();

            Mock<IDataRepositoryRetriever> mockServoRepoRetriever;
            When(Method(mockServoRepoRetriever, get)).AlwaysReturn(pServerRepo);

            SyncObexHandler handler(serverDeviceId, &mockServoRepoRetriever.get(), CString(), nullptr);

            // Add a few initial cases to the client
            VectorClock clockClient1;
            clockClient1.increment(clientDeviceId);
            auto clientCase1 = makeCase(*case_access, L"guid1", 1, { L"initialclientdata1" });
            clientCase1->SetVectorClock(clockClient1);
            auto clientCase2 = makeCase(*case_access, L"guid2", 2, { L"initialclientdata2" });
            clientCase2->SetVectorClock(clockClient1);

            // Put cases to server
            CString syncPath = L"/dictionaries/" + dictionary + L"/syncs";
            HeaderList requestHeaders;
            CString userAgentHeader = CString("User-Agent: CSPro sync client/") + Versioning::GetVersionDetailedString();
            requestHeaders.push_back(userAgentHeader);
            requestHeaders.push_back(SyncCustomHeaders::DEVICE_ID_HEADER + _T(": ") + clientDeviceId);
            std::string putJson = convertCasesToJSON(jsonConverter, { clientCase1, clientCase2 });
            std::unique_ptr<IObexResource> resource;
            ObexResponseCode responseCode = handler.onPut(OBEX_SYNC_DATA_MEDIA_TYPE, syncPath, requestHeaders, resource);
            Assert::AreEqual((int)OBEX_OK, (int)responseCode);
            resource->openForWriting();
            resource->getOStream()->write(&putJson[0], putJson.size());
            Assert::AreEqual((int)OBEX_OK, (int)resource->openForReading());
            Assert::IsNull(resource->getIStream());
            Assert::AreEqual((int)OBEX_OK, (int)resource->close());
            HeaderList responseHeaders = resource->getHeaders();
            Assert::IsFalse(responseHeaders.value(_T("ETag")).IsEmpty(), L"Response missing etag");
            CString serverRevisionFromPut = responseHeaders.value(_T("ETag"));
            resource.reset();

            // Delete server history and use etag, should get precondition failed
            serverRepoBuilder.ResetRepo(serverDeviceId, pDict);
            pServerRepo = serverRepoBuilder.GetRepo();
            When(Method(mockServoRepoRetriever, get)).AlwaysReturn(pServerRepo);

            requestHeaders.push_back(SyncCustomHeaders::IF_REVISION_EXISTS_HEADER + _T(':') + serverRevisionFromPut);
            responseCode = handler.onPut(OBEX_SYNC_DATA_MEDIA_TYPE, syncPath, requestHeaders, resource);
            Assert::AreEqual((int)OBEX_OK, (int)responseCode);
            Assert::AreEqual((int)OBEX_PRECONDITION_FAILED, (int)resource->openForWriting());
            resource.reset();

            // Add a case on server from a different client
            auto clientCase3 = makeCase(*case_access, L"guid3", 3, { L"13newclientdata3" });
            clientCase3->SetVectorClock(clockClient1);
            putJson = convertCasesToJSON(jsonConverter, { clientCase3 });
            HeaderList requestHeadersClient2;
            requestHeadersClient2.push_back(SyncCustomHeaders::DEVICE_ID_HEADER + _T(": client2"));
            responseCode = handler.onPut(OBEX_SYNC_DATA_MEDIA_TYPE, syncPath, requestHeadersClient2, resource);
            Assert::AreEqual((int)OBEX_OK, (int)responseCode);
            resource->openForWriting();
            resource->getOStream()->write(&putJson[0], putJson.size());
            Assert::AreEqual((int)OBEX_OK, (int)resource->openForReading());
            Assert::IsNull(resource->getIStream());
            Assert::AreEqual((int)OBEX_OK, (int)resource->close());
            responseHeaders = resource->getHeaders();
            Assert::IsFalse(responseHeaders.value(_T("ETag")).IsEmpty(), L"Response missing etag");
            CString serverRevisionFromPut2 = responseHeaders.value(_T("ETag"));
            resource.reset();

            // Sync again from client1 - should still fail on precondition
            responseCode = handler.onPut(OBEX_SYNC_DATA_MEDIA_TYPE, syncPath, requestHeaders, resource);
            Assert::AreEqual((int)OBEX_OK, (int)responseCode);
            Assert::AreEqual((int)OBEX_PRECONDITION_FAILED, (int)resource->openForWriting());
            resource.reset();

        }

        TEST_METHOD(TestSyncResumableGet)
        {
            std::unique_ptr<const CDataDict> pDictAP = createTestDict();
            const CDataDict* pDict = pDictAP.get();
            std::shared_ptr<CaseAccess> case_access = CaseAccess::CreateAndInitializeFullCaseAccess(*pDict);
            TestRepoBuilder serverRepoBuilder(serverDeviceId, pDict);
            ISyncableDataRepository* pServerRepo = serverRepoBuilder.GetRepo();

            Mock<IDataRepositoryRetriever> mockRepoRetriever;
            When(Method(mockRepoRetriever, get)).AlwaysReturn(pServerRepo);

            SyncObexHandler handler(serverDeviceId, &mockRepoRetriever.get(), CString(), nullptr);

            CString syncPath = L"/dictionaries/" + dictionary + L"/syncs";
            std::unique_ptr<IObexResource> resource;

            std::vector<std::shared_ptr<Case>> serverCases;
            std::vector<std::shared_ptr<Case>> clientCases;

            // Start with 30 cases on server
            VectorClock clockServer;
            clockServer.increment(serverDeviceId);
            for (int i = 1; i <= 30; ++i) {
                CString uuid;
                uuid.Format(L"guid%02d", i);
                auto initServerCase = makeCase(*case_access, uuid, i, { L"initialserverdata" });
                initServerCase->SetVectorClock(clockServer);
                serverCases.emplace_back(initServerCase);
            }
            serverRepoBuilder.setInitialRepoCases(serverCases, serverDeviceId);

            // Get 10 cases
            HeaderList requestHeaders;
            CString userAgentHeader = CString("User-Agent: CSPro sync client/") + Versioning::GetVersionDetailedString();
            requestHeaders.push_back(userAgentHeader);
            requestHeaders.push_back(SyncCustomHeaders::DEVICE_ID_HEADER + _T(": ") + clientDeviceId);
            requestHeaders.push_back(SyncCustomHeaders::RANGE_COUNT_HEADER + _T(": ") + _T("10"));
            ObexResponseCode responseCode = handler.onGet(OBEX_SYNC_DATA_MEDIA_TYPE, syncPath, requestHeaders, resource);
            Assert::AreEqual((int) OBEX_OK, (int) responseCode);
            Assert::AreEqual((int) OBEX_PARTIAL_CONTENT, (int) resource->openForReading());
            std::string responseJson = std::string(std::istreambuf_iterator<char>(*resource->getIStream()), {});
            ZipUtility::decompression(responseJson);
            HeaderList responseHeaders = resource->getHeaders();
            Assert::IsFalse(responseHeaders.value(_T("ETag")).IsEmpty(), L"Response missing etag");
            resource.reset();
            auto responseCases = getCaseListFromJson(jsonConverter, responseJson, *case_access);
            Assert::AreEqual(10, (int) responseCases.size());
            mergeCaseList(clientCases, responseCases);

            // Get next 10 cases
            requestHeaders = HeaderList();
            requestHeaders.push_back(userAgentHeader);
            requestHeaders.push_back(SyncCustomHeaders::DEVICE_ID_HEADER + _T(": ") + clientDeviceId);
            requestHeaders.push_back(SyncCustomHeaders::RANGE_COUNT_HEADER + _T(": ") + _T("10"));
            requestHeaders.push_back(SyncCustomHeaders::START_AFTER_HEADER + _T(": ") + responseCases.back()->GetUuid());
            requestHeaders.push_back(SyncCustomHeaders::IF_REVISION_EXISTS_HEADER + _T(':') + responseHeaders.value(_T("ETag")));
            responseCode = handler.onGet(OBEX_SYNC_DATA_MEDIA_TYPE, syncPath, requestHeaders, resource);
            Assert::AreEqual((int) OBEX_OK, (int) responseCode);
            Assert::AreEqual((int) OBEX_PARTIAL_CONTENT, (int) resource->openForReading());
            responseJson = std::string(std::istreambuf_iterator<char>(*resource->getIStream()), {});
            ZipUtility::decompression(responseJson);
            responseHeaders = resource->getHeaders();
            Assert::IsFalse(responseHeaders.value(_T("ETag")).IsEmpty(), L"Response missing etag");
            resource.reset();
            responseCases = getCaseListFromJson(jsonConverter, responseJson, *case_access);
            Assert::AreEqual(10, (int) responseCases.size());
            mergeCaseList(clientCases, responseCases);

            // Add a new case on server and modify existing cases
            auto new_server_case = makeCase(*case_access, L"aaaa", 1, { L"addeddata" });
            new_server_case->SetVectorClock(clockServer);
            serverCases.emplace_back(new_server_case);
            serverRepoBuilder.addRepoCase(L"aaaa", 1, { L"addeddata" });
            serverRepoBuilder.updateRepoCase(serverCases[10]->GetUuid(), 10, L"updateddata");
            serverRepoBuilder.updateRepoCase(serverCases[25]->GetUuid(), 25, L"updateddata");

            // Get next 10 cases
            requestHeaders = HeaderList();
            requestHeaders.push_back(userAgentHeader);
            requestHeaders.push_back(SyncCustomHeaders::DEVICE_ID_HEADER + _T(": ") + clientDeviceId);
            requestHeaders.push_back(SyncCustomHeaders::RANGE_COUNT_HEADER + _T(": ") + _T("10"));
            requestHeaders.push_back(SyncCustomHeaders::START_AFTER_HEADER + _T(": ") + responseCases.back()->GetUuid());
            requestHeaders.push_back(SyncCustomHeaders::IF_REVISION_EXISTS_HEADER + _T(':') + responseHeaders.value(_T("ETag")));
            responseCode = handler.onGet(OBEX_SYNC_DATA_MEDIA_TYPE, syncPath, requestHeaders, resource);
            Assert::AreEqual((int) OBEX_OK, (int) responseCode);
            Assert::AreEqual((int) OBEX_PARTIAL_CONTENT, (int) resource->openForReading());
            responseJson = std::string(std::istreambuf_iterator<char>(*resource->getIStream()), {});
            ZipUtility::decompression(responseJson);
            responseHeaders = resource->getHeaders();
            Assert::IsFalse(responseHeaders.value(_T("ETag")).IsEmpty(), L"Response missing etag");
            resource.reset();
            responseCases = getCaseListFromJson(jsonConverter, responseJson, *case_access);
            Assert::AreEqual(10, (int) responseCases.size());
            mergeCaseList(clientCases, responseCases);

            // Get remaining 2 cases
            requestHeaders = HeaderList();
            requestHeaders.push_back(userAgentHeader);
            requestHeaders.push_back(SyncCustomHeaders::DEVICE_ID_HEADER + _T(": ") + clientDeviceId);
            requestHeaders.push_back(SyncCustomHeaders::RANGE_COUNT_HEADER + _T(": ") + _T("10"));
            requestHeaders.push_back(SyncCustomHeaders::START_AFTER_HEADER + _T(": ") + responseCases.back()->GetUuid());
            requestHeaders.push_back(SyncCustomHeaders::IF_REVISION_EXISTS_HEADER + _T(':') + responseHeaders.value(_T("ETag")));
            responseCode = handler.onGet(OBEX_SYNC_DATA_MEDIA_TYPE, syncPath, requestHeaders, resource);
            Assert::AreEqual((int) OBEX_OK, (int) responseCode);
            Assert::AreEqual((int) OBEX_OK, (int) resource->openForReading());
            responseJson = std::string(std::istreambuf_iterator<char>(*resource->getIStream()), {});
            ZipUtility::decompression(responseJson);
            responseHeaders = resource->getHeaders();
            Assert::IsFalse(responseHeaders.value(_T("ETag")).IsEmpty(), L"Response missing etag");
            resource.reset();
            responseCases = getCaseListFromJson(jsonConverter, responseJson, *case_access);

            Assert::AreEqual(2, (int) responseCases.size());
            mergeCaseList(clientCases, responseCases);

            checkCaseList(serverCases, clientCases);
        }

        TEST_METHOD(TestGetFile)
        {
            wchar_t pszTempPath[MAX_PATH];
            GetTempPath(MAX_PATH, pszTempPath);
            CString rootPath = CString(pszTempPath) + L"syncobexhandlertest/";
            PortableFunctions::DirectoryDelete(rootPath);
            CreateDirectory(rootPath, NULL);

            const std::string fileContents("SOME DATA");
            CString testFile = "test-get.txt";
            createTestFile(rootPath + testFile, fileContents.c_str());

            SyncObexHandler handler(serverDeviceId, nullptr, rootPath, nullptr);

            std::unique_ptr<IObexResource> resource;
            ObexResponseCode responseCode = handler.onGet(OBEX_BINARY_FILE_MEDIA_TYPE, testFile, HeaderList(), resource);
            Assert::AreEqual((int)OBEX_OK, (int)responseCode);
            Assert::AreEqual((int)OBEX_IS_LAST_FILE_CHUNK, (int)resource->openForReading());

            std::string response(std::istreambuf_iterator<char>(*resource->getIStream()), {});
            ZipUtility::decompression(response);
            Assert::AreEqual(fileContents, response);
            resource.reset();

            // Etag should return not-modified
            HeaderList requestHeaders;
            CString userAgentHeader = CString("User-Agent: CSPro sync client/") + Versioning::GetVersionDetailedString();
            requestHeaders.push_back(userAgentHeader);
            requestHeaders.push_back(_T("If-None-Match:662411C1698ECC13DD07AEE13439EADC"));
            responseCode = handler.onGet(OBEX_BINARY_FILE_MEDIA_TYPE, testFile, requestHeaders, resource);
            Assert::AreEqual((int)OBEX_NOT_MODIFIED, (int)responseCode);

            // Non-matching Etag should download
            requestHeaders = HeaderList();
            requestHeaders.push_back(userAgentHeader);
            requestHeaders.push_back(_T("If-None-Match:1234567890"));
            responseCode = handler.onGet(OBEX_BINARY_FILE_MEDIA_TYPE, testFile, requestHeaders, resource);
            Assert::AreEqual((int)OBEX_OK, (int)responseCode);
            resource.reset();

            // Non-existent file should fail
            responseCode = handler.onGet(OBEX_BINARY_FILE_MEDIA_TYPE, "file-that-doesnt-exist.txt", HeaderList(), resource);
            Assert::AreEqual((int)OBEX_NOT_FOUND, (int)responseCode);

            PortableFunctions::DirectoryDelete(rootPath);
        }

        TEST_METHOD(TestPutFile)
        {
            wchar_t pszTempPath[MAX_PATH];
            GetTempPath(MAX_PATH, pszTempPath);
            CString rootPath = CString(pszTempPath) + L"syncobexhandlertest/";
            PortableFunctions::DirectoryDelete(rootPath);
            CreateDirectory(rootPath, nullptr);

            const std::string fileContents("SOME DATA");
            CString testFile = "test-put.txt";

            SyncObexHandler handler(serverDeviceId, nullptr, rootPath, nullptr);

            std::unique_ptr<IObexResource> resource;
            ObexResponseCode responseCode = handler.onPut(OBEX_BINARY_FILE_MEDIA_TYPE, testFile, HeaderList(), resource);
            Assert::AreEqual((int)OBEX_OK, (int)responseCode);
            Assert::AreEqual((int)OBEX_OK, (int)resource->openForWriting());

            resource->getOStream()->write(&fileContents[0], fileContents.size());
            Assert::IsFalse(resource->getOStream()->fail());
            Assert::AreEqual((int) OBEX_OK, (int) resource->close());
            resource.reset();

            CString testFileFullPath = rootPath + testFile;
            Assert::IsTrue(GetFileAttributes(testFileFullPath) != -1);
            std::ifstream srcReadStream(testFileFullPath);
            std::string actualContents(std::istreambuf_iterator<char>(srcReadStream), {});
            srcReadStream.close();
            Assert::AreEqual(actualContents, fileContents);

            PortableFunctions::DirectoryDelete(rootPath);
        }

        TEST_METHOD(TestGetDirectoryListing)
        {
            wchar_t pszTempPath[MAX_PATH];
            GetTempPath(MAX_PATH, pszTempPath);
            CString rootPath = CString(pszTempPath) + L"syncobexhandlertest/";
            PortableFunctions::DirectoryDelete(rootPath,true);
            CreateDirectory(rootPath, NULL);

            const std::string fileContents("SOME DATA");
            CString testFile1 = "test1.txt";
            CString testFile2 = "test2.txt";
            createTestFile(rootPath + testFile1, fileContents.c_str());
            createTestFile(rootPath + testFile2, fileContents.c_str());
            CString testDir = "test3-dir";
            CreateDirectory(rootPath + testDir, NULL);

            SyncObexHandler handler(serverDeviceId, nullptr, rootPath, nullptr);

            std::unique_ptr<IObexResource> resource;
            ObexResponseCode responseCode = handler.onGet(OBEX_DIRECTORY_LISTING_MEDIA_TYPE, ".", HeaderList(), resource);
            Assert::AreEqual((int)OBEX_OK, (int)responseCode);
            Assert::AreEqual((int)OBEX_OK, (int)resource->openForReading());

            std::string responseJson;
            responseJson = std::string(std::istreambuf_iterator<char>(*resource->getIStream()), {});
            resource.reset();

            std::unique_ptr<std::vector<FileInfo>> pListing(jsonConverter.fileInfoFromJson(responseJson));
            Assert::IsNotNull(pListing.get());

            Assert::AreEqual(testFile1, pListing->at(0).getName());
            Assert::AreEqual(L"/.", pListing->at(0).getDirectory());
            Assert::AreEqual(L"662411c1698ecc13dd07aee13439eadc", pListing->at(0).getMd5().c_str());
            Assert::AreEqual((int) FileInfo::FileType::File, (int) pListing->at(0).getType());
            Assert::AreEqual((int) fileContents.size(), (int) pListing->at(0).getSize());
            Assert::AreEqual(testFile2, pListing->at(1).getName());
            Assert::AreEqual(L"/.", pListing->at(1).getDirectory());
            Assert::AreEqual(L"662411c1698ecc13dd07aee13439eadc", pListing->at(1).getMd5().c_str());
            Assert::AreEqual((int)FileInfo::FileType::File, (int)pListing->at(1).getType());
            Assert::AreEqual((int)fileContents.size(), (int) pListing->at(1).getSize());
            Assert::AreEqual(testDir, pListing->at(2).getName());
            Assert::AreEqual((int)FileInfo::FileType::Directory, (int)pListing->at(2).getType());

            // Non-existent directory should fail
            responseCode = handler.onGet(OBEX_DIRECTORY_LISTING_MEDIA_TYPE, "directory-that-doesnt-exist.txt", HeaderList(), resource);
            Assert::AreEqual((int)OBEX_NOT_FOUND, (int)responseCode);

            PortableFunctions::DirectoryDelete(rootPath);
        }
    };
}

