#include "stdafx.h"
#include "CaseTestHelpers.h"
#include "CSWebSyncServiceWithModifiableApi.h"
#include "SyncTestCredentials.h"
#include "TestRepoBuilder.h"
#include <zToolsO/ApiKeys.h>
#include <zToolsO/VectorHelpers.h>
#include <zUtilO/CredentialStore.h>
#include <zUtilO/TemporaryFile.h>
#include <zNetwork/CurlHttpConnection.h>
#include <zDictO/DDClass.h>
#include <zDataO/CSWebRepository.h>
#include <zSyncO/CaseObservable.h>
#include <zSyncO/CSWebSyncService.h>
#include <fstream>

using namespace fakeit;


namespace SyncUnitTest
{
    std::string random_string(const size_t length)
    {
        constexpr char charset[] = "0123456789"
                                   "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                   "abcdefghijklmnopqrstuvwxyz";

        std::string str;
        for (size_t i = 0; i < length; ++i) {
            str.push_back(charset[rand() % (sizeof(charset)/sizeof(charset[0]) - 1)]);
        }
        return str;
    }

    TEST_CLASS(CSWebSyncServiceTest)
    {
    private:
        const std::string hostUrl = "http://test.com/";
        const UsernamePassword username_password { "username", "password" };
        const DeviceId clientDeviceId = "clientDeviceId";
        const DeviceId serverDeviceId = "serverDeviceId";
        const std::string universe = "universe";

        const std::unique_ptr<const CDataDict> dictionary = CreateTestDictionary();
        const std::shared_ptr<CaseAccess> case_access = CaseAccess::CreateAndInitializeFullCaseAccess(*dictionary);
        const std::string dictName = dictionary->GetSyncableName();

        std::string createNumberedCaseGuid(int n)
        {
            return FormatText("guid%06d", n);
        }

        std::vector<std::shared_ptr<Case>> createClientTestCases()
        {
            VectorClock clientCaseClock;
            clientCaseClock.increment(clientDeviceId);
            std::shared_ptr<Case> clientCase1 = CreateCase(*case_access, "guidc1", 1, { "clientdata1" }, false);
            clientCase1->SetVectorClock(clientCaseClock);
            std::shared_ptr<Case> clientCase2 = CreateCase(*case_access, "guidc2", 2, { "clientdata2" }, false);
            clientCase2->SetVectorClock(clientCaseClock);
            return { clientCase1, clientCase2 };
        }

        std::vector<std::shared_ptr<Case>> createServerTestCases()
        {
            VectorClock serverCaseClock;
            serverCaseClock.increment(serverDeviceId);
            std::shared_ptr<Case> serverCase1 = CreateCase(*case_access, "guids1", 1, { "serverdata1" }, false);
            serverCase1->SetVectorClock(serverCaseClock);
            std::shared_ptr<Case> serverCase2 = CreateCase(*case_access, "guids2", 2, { "serverdata2" }, false);
            serverCase2->SetVectorClock(serverCaseClock);
            return { serverCase1, serverCase2 };
        }

        std::vector<std::shared_ptr<Case>> serverCases = createServerTestCases();

        std::vector<std::shared_ptr<Case>> clientCases = createClientTestCases();

        const int MAX_RETRIES = 3;

        static std::string stripQuotes(std::string s)
        {
            if( s.length() >= 2 && s.front() == '\"' && s.back() == '\"' )
                return s.substr(1, s.length() - 2);

            return s;
        }

    public:
        TEST_METHOD(TestSyncDataFileGet)
        {
            Mock<HttpConnection> mockHttp;

            std::string serverCasesJson = GetSyncableCaseData(case_access, SpanHelpers::CreatePointersSpan(serverCases), SyncCaseSerializer::Version::V2);

            const std::string serverRevision = "1";

            When(Method(mockHttp, Request)).
                Do([serverCasesJson, serverRevision, this](const HttpRequest& request) {
                const std::string expectedUrl = hostUrl + "dictionaries/" + dictName + "/cases";
                Assert::AreEqual(expectedUrl, request.url, L"URL does not match");
                Assert::AreEqual(clientDeviceId, request.headers.GetValue("x-csw-device"));
                Assert::AreEqual(universe, stripQuotes(request.headers.GetValue("x-csw-universe")));
                Assert::AreEqual(SO::Empty_string, request.headers.GetValue("x-csw-if-revision-exists"));
                HttpResponse response(HttpResponse::Status_200_OK);
                response.headers.Add("Etag", serverRevision);
                response.body.observable = rxcpp::observable<>::just(serverCasesJson);
                return response;
            });

            CSWebSyncService sync_service(std::unique_ptr<HttpConnection>(&mockHttp.get()), hostUrl, username_password);

            SyncGetResponse response = sync_service.GetCases(case_access, clientDeviceId, universe, std::string(), std::string(), std::vector<std::string>());
            Assert::AreEqual(SyncGetResponse::SyncGetResult::Complete, response.GetResult());
            Assert::AreEqual(serverRevision, response.GetServerRevision());
            CompareCases(SpanHelpers::CreatePointersSpan(serverCases),
                         SpanHelpers::CreatePointersSpan(ObservableToVector(*response.GetCases())),
                         CompareCasesType::Counts);
            Verify(Method(mockHttp, Request)).Once();
        }


        TEST_METHOD(TestSyncDataFileGetBadJson)
        {
            Mock<HttpConnection> mockHttp;

            When(Method(mockHttp, Request)).
                Do([](const HttpRequest&) {
                HttpResponse response(HttpResponse::Status_200_OK);
                response.headers.Add("Etag", "1");
                response.body.observable = rxcpp::observable<>::just(std::string("some totally invalid json"));
                return response;
            });

            CSWebSyncService sync_service(std::unique_ptr<HttpConnection>(&mockHttp.get()), hostUrl, username_password);

            SyncGetResponse response = sync_service.GetCases(case_access, clientDeviceId, universe, std::string(), std::string(), std::vector<std::string>());
            Assert::ExpectException<SyncError>([&]() { CompareCases(SpanHelpers::CreatePointersSpan(serverCases),
                                                                    SpanHelpers::CreatePointersSpan(ObservableToVector(*response.GetCases())),
                                                                    CompareCasesType::Counts); });
        }


        TEST_METHOD(TestSyncDataFilePut)
        {
            Mock<HttpConnection> mockHttp;

            SyncCaseSerializer sync_case_serializer_v2(case_access, SyncCaseSerializer::Version::V2);
            const std::string clientCasesJson = sync_case_serializer_v2.GetSyncableJson(SpanHelpers::CreatePointersSpan(clientCases));

            const std::string serverRevision = "1";

            When(Method(mockHttp, Request)).
                Do([](const HttpRequest&) {
                HttpResponse response(HttpResponse::Status_200_OK);
                response.body.observable = rxcpp::observable<>::just(std::string("{\"access_token\":\"foo\",\"expires_in\":3600,\"scope\":null,\"token_type\":\"Bearer\",\"refresh_token\":\"foo\"}"));
                return response;
            }).Do([](const HttpRequest&) {
                HttpResponse response(HttpResponse::Status_200_OK);
                response.body.observable = rxcpp::observable<>::just(std::string("{\"deviceId\":\"foo\",\"apiVersion\":2}"));
                return response;
            }).Do([clientCasesJson, serverRevision, this](const HttpRequest& request) {
                const std::string expectedUrl = hostUrl + "dictionaries/" + dictName + "/cases";
                Assert::AreEqual(expectedUrl, request.url, L"URL does not match");
                Assert::AreEqual(clientDeviceId, request.headers.GetValue("x-csw-device"));
                Assert::AreEqual(SO::Empty_string, request.headers.GetValue("x-csw-universe"));
                Assert::AreEqual(SO::Empty_string, request.headers.GetValue("x-csw-if-revision-exists"));
                std::string postDataJson(std::istreambuf_iterator<char>(*request.upload_data), {});
                ZLib::Inflate(postDataJson);
                Assert::AreEqual(clientCasesJson, postDataJson);
                HttpResponse response(HttpResponse::Status_200_OK);
                response.headers.Add("Etag", serverRevision);
                response.body.observable = rxcpp::observable<>::just(std::string("{\"code\":\"200\",\"message\":\"success\"}"));
                return response;
            });

            CSWebSyncService sync_service(std::unique_ptr<HttpConnection>(&mockHttp.get()), hostUrl, username_password);
            sync_service.Connect();

            const SyncPutResponse response = sync_service.PutCases(case_access, SpanHelpers::CreatePointersSpan(clientCases), nullptr, clientDeviceId, universe, std::string());
            Assert::AreEqual(SyncPutResponse::SyncPutResult::Complete, response.GetResult());
            Assert::AreEqual(serverRevision, response.GetServerRevision());
            Verify(Method(mockHttp, Request)).Exactly(3);
        }


        TEST_METHOD(TestSyncDataFileHttpError)
        {
            Mock<HttpConnection> mockHttp;

            std::string clientCasesJson = GetSyncableCaseData(case_access, SpanHelpers::CreatePointersSpan(clientCases), SyncCaseSerializer::Version::V2);

            const std::string getServerRevision = "1";

            When(Method(mockHttp, Request)).
                Do([](const HttpRequest&) {
                HttpResponse response(HttpResponse::Status_404_NotFound);
                response.body.observable = rxcpp::observable<>::just(std::string("{code=404, message=\"Not found\"}"));
                return response;
            });

            CSWebSyncService sync_service(std::unique_ptr<HttpConnection>(&mockHttp.get()), hostUrl, username_password);

            try
            {
                sync_service.PutCases(case_access, SpanHelpers::CreatePointersSpan(clientCases), nullptr, clientDeviceId, universe, std::string());
                Assert::Fail(L"Expected exception");
            }
            catch( const SyncError& ) { }

            Verify(Method(mockHttp, Request)).Once();
        }


        TEST_METHOD(TestSyncDataFileGetChunked)
        {
            Mock<HttpConnection> mockHttp;

            int totalCases = -1;
            int lastCaseNumSent = -1;
            int revisionNum = 0;
            std::vector<std::shared_ptr<Case>> allCases;
            std::vector<std::shared_ptr<Case>> responseCases;

            When(Method(mockHttp, Request)).
                AlwaysDo([this, &totalCases, &lastCaseNumSent, &revisionNum, &allCases](const HttpRequest& request) {
                const std::string startGuid = request.headers.GetValue("x-csw-case-range-start-after");
                if (lastCaseNumSent == -1) {
                    // First time should be no start after
                    Assert::AreEqual(std::string(), startGuid);
                } else {
                    Assert::AreEqual(createNumberedCaseGuid(lastCaseNumSent), startGuid);
                }
                int startCaseNum = lastCaseNumSent + 1;
                const std::string rangeCount = request.headers.GetValue("x-csw-case-range-count");
                Assert::AreNotEqual(std::string(), rangeCount);
                int numCasesToReturn = atoi(rangeCount.c_str());
                if (totalCases == -1)
                    totalCases = int(numCasesToReturn * 3.5); // so that we get 3 calls total with chunk 1: n, chunk 2: 2n, chunk 3: 0.5n

                if (lastCaseNumSent + numCasesToReturn > totalCases) {
                    numCasesToReturn = totalCases - startCaseNum;
                }
                VectorClock serverCaseClock;
                serverCaseClock.increment(serverDeviceId);
                std::vector<std::shared_ptr<Case>> casesThisChunk;
                for (int i = 0; i < numCasesToReturn; ++i) {
                    std::shared_ptr<Case> data_case = CreateCase(*case_access, createNumberedCaseGuid(i + startCaseNum), i, { "serverdata" }, false);
                    data_case->SetVectorClock(serverCaseClock);
                    casesThisChunk.emplace_back(data_case);
                }
                std::copy(casesThisChunk.begin(), casesThisChunk.end(), std::back_inserter(allCases));

                lastCaseNumSent += numCasesToReturn;
                HttpResponse response(lastCaseNumSent < totalCases - 1 ? HttpResponse::Status_206_PartialContent : HttpResponse::Status_200_OK);
                std::string casesJson = GetSyncableCaseData(case_access, SpanHelpers::CreatePointersSpan(casesThisChunk), SyncCaseSerializer::Version::V2);
                response.body.observable = rxcpp::observable<>::just(casesJson);

                response.headers.Add("x-csw-case-range-count", FormatText("%d/%d", numCasesToReturn, totalCases));

                // increase rev number to simulate other devices to syncing to server in between requests
                response.headers.Add("Etag", IntToString(++revisionNum));

                return response;
            });

            CSWebSyncService sync_service(std::unique_ptr<HttpConnection>(&mockHttp.get()), hostUrl, username_password);

            SyncGetResponse response = sync_service.GetCases(case_access, clientDeviceId, universe, std::string(), std::string(), std::vector<std::string>());
            Assert::AreEqual(SyncGetResponse::SyncGetResult::MoreData, response.GetResult());
            Assert::AreEqual(revisionNum, std::stoi(response.GetServerRevision()));
            VectorHelpers::Append(responseCases, ObservableToVector(*response.GetCases()));

            response = sync_service.GetCases(case_access, clientDeviceId, universe, response.GetServerRevision(),
                                             responseCases.back()->GetUuid(), std::vector<std::string>());
            Assert::AreEqual(SyncGetResponse::SyncGetResult::MoreData, response.GetResult());
            Assert::AreEqual(revisionNum, std::stoi(response.GetServerRevision()));
            VectorHelpers::Append(responseCases, ObservableToVector(*response.GetCases()));

            response = sync_service.GetCases(case_access, clientDeviceId, universe, response.GetServerRevision(),
                                             responseCases.back()->GetUuid(), std::vector<std::string>());
            Assert::AreEqual(SyncGetResponse::SyncGetResult::Complete, response.GetResult());
            Assert::AreEqual(revisionNum, std::stoi(response.GetServerRevision()));
            VectorHelpers::Append(responseCases, ObservableToVector(*response.GetCases()));

            CompareCases(SpanHelpers::CreatePointersSpan(allCases),
                         SpanHelpers::CreatePointersSpan(responseCases),
                         CompareCasesType::Counts);

            Verify(Method(mockHttp, Request)).Exactly(3);
        }


        TEST_METHOD(TestSyncDataFileGetAdaptiveChunkSize)
        {
            Mock<HttpConnection> mockHttp;

            const int initialChunkSize = 100;
            int totalCases = initialChunkSize * 10;
            int nextCaseNum = 1;
            int requests = 0;

            When(Method(mockHttp, Request)).
                AlwaysDo([this, initialChunkSize, &requests, &totalCases, &nextCaseNum](const HttpRequest& request) {

                const std::string rangeCount = request.headers.GetValue("x-csw-case-range-count");
                Assert::AreNotEqual(std::string(), rangeCount);
                int chunkSizeRequested = atoi(rangeCount.c_str());
                size_t caseDataSize = 100;

                ++requests;

                if (requests == 1) {
                    Assert::AreEqual(initialChunkSize, chunkSizeRequested);
                }
                else if (requests == 2) {
                    Assert::AreEqual(initialChunkSize * 2, chunkSizeRequested); // chunk size should have doubled
                    Sleep(30010);
                }
                else if (requests == 3) {
                    // Should still be 200 since last request took more than 30 secs
                    Assert::AreEqual(initialChunkSize * 2, chunkSizeRequested);
                }
                else if (requests == 4) {
                    // Should double again
                    Assert::AreEqual(initialChunkSize * 4, chunkSizeRequested);
                    caseDataSize = int((10 * 1e+6) / chunkSizeRequested);
                }
                else if (requests == 5) {
                    // Should stay same because size is big
                    Assert::AreEqual(initialChunkSize * 4, chunkSizeRequested);
                }

                VectorClock serverCaseClock;
                serverCaseClock.increment(serverDeviceId);
                std::vector<std::shared_ptr<Case>> casesThisChunk;
                for (int i = 0; i < chunkSizeRequested; ++i) {
                    ++nextCaseNum;
                    std::vector<std::string> caseData;
                    do {
                        caseData.emplace_back(random_string(30));
                    } while (caseData.size() * 30 < caseDataSize);

                    std::shared_ptr<Case> data_case = CreateCase(*case_access, createNumberedCaseGuid(nextCaseNum), i, caseData, false);
                    data_case->SetVectorClock(serverCaseClock);
                    casesThisChunk.emplace_back(data_case);
                }

                HttpResponse response(requests < 5 ? HttpResponse::Status_206_PartialContent : HttpResponse::Status_200_OK);

                std::string casesJson = GetSyncableCaseData(case_access, SpanHelpers::CreatePointersSpan(casesThisChunk), SyncCaseSerializer::Version::V2);
                response.body.observable = rxcpp::observable<>::just(casesJson);

                response.headers.Add("x-csw-case-range-count", FormatText("%d/%d", chunkSizeRequested, totalCases));
                response.headers.Add("Etag", "1");

                return response;
            });

            CSWebSyncService sync_service(std::unique_ptr<HttpConnection>(&mockHttp.get()), hostUrl, username_password);

            SyncGetResponse::SyncGetResult result;

            do {
                SyncGetResponse response = sync_service.GetCases(case_access, clientDeviceId, universe, std::string(), std::string(), std::vector<std::string>());
                ObservableToVector(*response.GetCases());
                result = response.GetResult();
            } while (result == SyncGetResponse::SyncGetResult::MoreData);
        }


        TEST_METHOD(TestSyncDataFileGetAfterServerReset)
        {
            Mock<HttpConnection> mockHttp;
            const std::string lastServerRevision = "5";

            When(Method(mockHttp, Request)).
                Do([lastServerRevision, this](const HttpRequest& request) {
                const std::string expectedUrl = hostUrl + "dictionaries/" + dictName + "/cases";
                Assert::AreEqual(expectedUrl, request.url, L"URL does not match");
                Assert::AreEqual(clientDeviceId, request.headers.GetValue("x-csw-device"));
                Assert::AreEqual(universe, stripQuotes(request.headers.GetValue("x-csw-universe")));
                Assert::AreEqual(lastServerRevision, request.headers.GetValue("x-csw-if-revision-exists"));
                return HttpResponse(412);
            });

            CSWebSyncService sync_service(std::unique_ptr<HttpConnection>(&mockHttp.get()), hostUrl, username_password);

            const SyncGetResponse response = sync_service.GetCases(case_access, clientDeviceId, universe, lastServerRevision, std::string(), std::vector<std::string>());
            Assert::AreEqual(SyncGetResponse::SyncGetResult::RevisionNotFound, response.GetResult());
            Verify(Method(mockHttp, Request)).Once();
        }


        TEST_METHOD(TestSyncDataFilePutAfterServerReset)
        {
            Mock<HttpConnection> mockHttp;

            const std::string lastServerRevision = "5";

            When(Method(mockHttp, Request)).
                Do([lastServerRevision, this](const HttpRequest& request) {
                const std::string expectedUrl = hostUrl + "dictionaries/" + dictName + "/cases";
                Assert::AreEqual(expectedUrl, request.url, L"URL does not match");
                Assert::AreEqual(clientDeviceId, request.headers.GetValue("x-csw-device"));
                Assert::AreEqual(SO::Empty_string, request.headers.GetValue("x-csw-universe"));
                Assert::AreEqual(lastServerRevision, request.headers.GetValue("x-csw-if-revision-exists"));
                return HttpResponse(412);
            });

            CSWebSyncService sync_service(std::unique_ptr<HttpConnection>(&mockHttp.get()), hostUrl, username_password);

            SyncPutResponse response = sync_service.PutCases(case_access, SpanHelpers::CreatePointersSpan(clientCases), nullptr, clientDeviceId, universe, lastServerRevision);
            Assert::AreEqual(SyncPutResponse::SyncPutResult::RevisionNotFound, response.GetResult());
            Verify(Method(mockHttp, Request)).Once();
        }


        TEST_METHOD(TestSyncDataFileRefreshAuthToken)
        {
            Mock<HttpConnection> mockHttp;

            SyncCaseSerializer sync_case_serializer_v2(case_access, SyncCaseSerializer::Version::V2);
            const std::string clientCasesJson = sync_case_serializer_v2.GetSyncableJson(SpanHelpers::CreatePointersSpan(clientCases));
            const std::string serverCasesJson = sync_case_serializer_v2.GetSyncableJson(SpanHelpers::CreatePointersSpan(serverCases));

            const std::string getServerRevision = "1";
            const std::string postServerRevision = "2";

            const std::string accessToken1 = "accessToken1";
            const std::string refreshToken1 = "refreshToken1";
            const std::string accessToken2 = "accessToken2";
            const std::string refreshToken2 = "refreshToken2";
            const std::string accessToken3 = "accessToken3";
            const std::string refreshToken3 = "refreshToken3";

            When(Method(mockHttp, Request)).
                Do([accessToken1, refreshToken1, this](const HttpRequest& request) {
                // First post will be to token endpoint
                Assert::IsTrue(request.method == HttpRequestMethod::HTTP_POST);
                const std::string expectedUrl = hostUrl + "token";
                Assert::AreEqual(expectedUrl, request.url, L"URL does not match");

                const std::string expectedTokenRequest = FormatText("{\"client_id\":\"%s\",\"client_secret\":\"%s\",\"grant_type\":\"password\",\"username\":\"%s\",\"password\":\"%s\"}",
                                                                    CSWebKeys::client_id, CSWebKeys::client_secret,
                                                                    username_password.username.c_str(), username_password.password.c_str());
                std::string postDataJson(std::istreambuf_iterator<char>(*request.upload_data), {});
                Assert::AreEqual(expectedTokenRequest, SO::RemoveWhitespace(postDataJson));
                HttpResponse response(HttpResponse::Status_200_OK);
                std::ostringstream oss;
                oss << "{\"access_token\":\"" << accessToken1 << "\",\"expires_in\":3600,\"token_type\":\"Bearer\",\"scope\":null,\"refresh_token\":\"" << refreshToken1 << "\"}";
                response.body.observable = rxcpp::observable<>::just(oss.str());
                return response;
             }).
                Do([accessToken1, this](const HttpRequest& request) {
                // First get is get server info
                Assert::IsTrue(request.method == HttpRequestMethod::HTTP_GET);
                const std::string expectedUrl = hostUrl + "server";
                Assert::AreEqual(expectedUrl, request.url, L"URL does not match");
                Assert::AreEqual("Bearer " + accessToken1, request.headers.GetValue("Authorization"));
                HttpResponse response(HttpResponse::Status_200_OK);
                std::ostringstream oss;
                oss << "{\"deviceId\":\"" << serverDeviceId << "\", \"apiVersion\":2.0}";
                response.body.observable = rxcpp::observable<>::just(oss.str());
                return response;
            }).
                Do([accessToken1, this](const HttpRequest& request) {
                // Simulate expired token, force refresh
                Assert::IsTrue(request.method == HttpRequestMethod::HTTP_GET);
                const std::string expectedUrl = hostUrl + "dictionaries/" + dictName + "/cases";
                Assert::AreEqual(expectedUrl, request.url, L"URL does not match");
                Assert::AreEqual("Bearer " + accessToken1, request.headers.GetValue("Authorization"));
                return HttpResponse(HttpResponse::Status_401_Unauthorized);
            }).
                Do([refreshToken1, accessToken2, refreshToken2](const HttpRequest& request) {
                    // Second post is refresh token
                    Assert::IsTrue(request.method == HttpRequestMethod::HTTP_POST);
                    const std::string expectedTokenRequest = FormatText("{\"client_id\":\"%s\",\"client_secret\":\"%s\",\"grant_type\":\"refresh_token\",\"refresh_token\":\"%s\"}",
                                                                        CSWebKeys::client_id, CSWebKeys::client_secret, refreshToken1.c_str());
                    std::string postDataJson(std::istreambuf_iterator<char>(*request.upload_data), {});
                    Assert::AreEqual(expectedTokenRequest, SO::RemoveWhitespace(postDataJson));
                    HttpResponse response(HttpResponse::Status_200_OK);
                    std::ostringstream oss;
                    oss << "{\"access_token\":\"" << accessToken2 << "\",\"expires_in\":3600,\"token_type\":\"Bearer\",\"scope\":null,\"refresh_token\":\"" << refreshToken2 << "\"}";
                    response.body.observable = rxcpp::observable<>::just(oss.str());
                    return response;
             }).
                Do([accessToken2, serverCasesJson, getServerRevision, this](const HttpRequest& request) {
                 // get with the refresh token
                Assert::IsTrue(request.method == HttpRequestMethod::HTTP_GET);
                const std::string expectedUrl = hostUrl + "dictionaries/" + dictName + "/cases";
                Assert::AreEqual(expectedUrl, request.url, L"URL does not match");
                Assert::AreEqual("Bearer " + accessToken2, request.headers.GetValue("Authorization"));
                Assert::AreEqual(clientDeviceId, request.headers.GetValue("x-csw-device"));
                Assert::AreEqual(universe, stripQuotes(request.headers.GetValue("x-csw-universe")));
                Assert::AreEqual(SO::Empty_string, request.headers.GetValue("x-csw-if-revision-exists"));
                HttpResponse response(HttpResponse::Status_200_OK);
                response.headers.Add("Etag", getServerRevision);
                response.body.observable = rxcpp::observable<>::just(serverCasesJson);
                return response;
            }).
               Do([accessToken2, this](const HttpRequest& request) {
                // Third post is case upload
                Assert::IsTrue(request.method == HttpRequestMethod::HTTP_POST);
                const std::string expectedUrl = hostUrl + "dictionaries/" + dictName + "/cases";
                Assert::AreEqual(expectedUrl, request.url);
                Assert::AreEqual("Bearer " + accessToken2, request.headers.GetValue("Authorization"));
                // Simulate expired token, force refresh
                return HttpResponse(HttpResponse::Status_401_Unauthorized);
            }).Do([refreshToken2, accessToken3, refreshToken3](const HttpRequest& request) {
                // Fourth post is refresh token again
                Assert::IsTrue(request.method == HttpRequestMethod::HTTP_POST);
                const std::string expectedTokenRequest = FormatText("{\"client_id\":\"%s\",\"client_secret\":\"%s\",\"grant_type\":\"refresh_token\",\"refresh_token\":\"%s\"}",
                                                                    CSWebKeys::client_id, CSWebKeys::client_secret, refreshToken2.c_str());
                std::string postDataJson(std::istreambuf_iterator<char>(*request.upload_data), {});
                Assert::AreEqual(expectedTokenRequest, SO::RemoveWhitespace(postDataJson));
                HttpResponse response(HttpResponse::Status_200_OK);
                std::ostringstream oss;
                oss << "{\"access_token\":\"" << accessToken3 << "\",\"expires_in\":3600,\"token_type\":\"Bearer\",\"scope\":null,\"refresh_token\":\"" << refreshToken3 << "\"}";
                response.body.observable = rxcpp::observable<>::just(oss.str());
                return response;
            }).Do([accessToken3, clientCasesJson, postServerRevision, this](const HttpRequest& request) {
                // Fifth post is case upload again after getting refresh token
                Assert::IsTrue(request.method == HttpRequestMethod::HTTP_POST);
                const std::string expectedUrl = hostUrl + "dictionaries/" + dictName + "/cases";
                Assert::AreEqual(expectedUrl, request.url, L"URL does not match");
                Assert::AreEqual("Bearer " + accessToken3, request.headers.GetValue("Authorization"));
                Assert::AreEqual(clientDeviceId, request.headers.GetValue("x-csw-device"));
                Assert::AreEqual(SO::Empty_string, request.headers.GetValue("x-csw-universe"));
                Assert::AreEqual(SO::Empty_string, request.headers.GetValue("x-csw-if-revision-exists"));
                std::string postDataJson(std::istreambuf_iterator<char>(*request.upload_data), {});
                ZLib::Inflate(postDataJson);
                Assert::AreEqual(clientCasesJson, postDataJson);
                HttpResponse response(HttpResponse::Status_200_OK);
                response.headers.Add("Etag", postServerRevision);
                response.body.observable = rxcpp::observable<>::just(std::string("{\"code\":\"200\",\"message\":\"success\"}"));
                return response;
                });

            CSWebSyncService sync_service(std::unique_ptr<HttpConnection>(&mockHttp.get()), hostUrl, username_password);

            std::shared_ptr<ConnectResponse> pConnectResponse(sync_service.Connect());
            Assert::AreEqual(serverDeviceId, pConnectResponse->GetServerDeviceId());

            SyncGetResponse getResponse = sync_service.GetCases(case_access, clientDeviceId, universe, std::string(), std::string(), std::vector<std::string>());
            Assert::AreEqual(SyncGetResponse::SyncGetResult::Complete, getResponse.GetResult());
            Assert::AreEqual(getServerRevision, getResponse.GetServerRevision());
            CompareCases(SpanHelpers::CreatePointersSpan(serverCases),
                         SpanHelpers::CreatePointersSpan(ObservableToVector(*getResponse.GetCases())),
                         CompareCasesType::Counts);

            SyncPutResponse putResponse = sync_service.PutCases(case_access, SpanHelpers::CreatePointersSpan(clientCases), nullptr, clientDeviceId, universe, std::string());
            Assert::AreEqual(SyncPutResponse::SyncPutResult::Complete, putResponse.GetResult());
            Assert::AreEqual(postServerRevision, putResponse.GetServerRevision());

            Verify(Method(mockHttp, Request)).Exactly(8);
        }


        TEST_METHOD(TestSyncDataFileGetRetry)
        {
            {
                Mock<HttpConnection> mockHttp;
                When(Method(mockHttp, Request)).
                    AlwaysDo([](HttpRequest) -> HttpResponse {
                    throw SyncRetryableNetworkError(1, "Retryable Error");
                });

                CSWebSyncService sync_service(std::unique_ptr<HttpConnection>(&mockHttp.get()), hostUrl, username_password);

                try {
                    sync_service.GetCases(case_access, clientDeviceId, universe, std::string(), std::string(), std::vector<std::string>());
                    Assert::Fail(L"Should have gotten an exception");
                }
                catch (const SyncError&) { }

                Verify(Method(mockHttp, Request)).Exactly(MAX_RETRIES + 1);
            }

            {
                Mock<HttpConnection> mockHttp;
                When(Method(mockHttp, Request)).
                    AlwaysDo([this](const HttpRequest&) {
                    return HttpResponse(500);
                });

                CSWebSyncService sync_service(std::unique_ptr<HttpConnection>(&mockHttp.get()), hostUrl, username_password);

                try {
                    sync_service.GetCases(case_access, clientDeviceId, universe, std::string(), std::string(), std::vector<std::string>());
                    Assert::Fail(L"Should have gotten an exception");
                }
                catch (const SyncError&) { }

                Verify(Method(mockHttp, Request)).Exactly(MAX_RETRIES + 1);
            }

            std::string serverCasesJson = GetSyncableCaseData(case_access, SpanHelpers::CreatePointersSpan(serverCases), SyncCaseSerializer::Version::V2);

            {
                Mock<HttpConnection> mockHttp;
                When(Method(mockHttp, Request)).
                    Do([](const HttpRequest&) {
                    return HttpResponse(500);
                }).Do([](const HttpRequest&) {
                    return HttpResponse(500);
                }).Do([](const HttpRequest&) {
                        return HttpResponse(500);
                }).Do([serverCasesJson](const HttpRequest&) {
                    HttpResponse response(HttpResponse::Status_200_OK);
                    response.headers.Add("Etag", "1");
                    response.body.observable = rxcpp::observable<>::just(serverCasesJson);
                    return response;
                });

                CSWebSyncService sync_service(std::unique_ptr<HttpConnection>(&mockHttp.get()), hostUrl, username_password);

                SyncGetResponse response = sync_service.GetCases(case_access, clientDeviceId, universe, std::string(), std::string(), std::vector<std::string>());
                Assert::AreEqual(SyncGetResponse::SyncGetResult::Complete, response.GetResult());
                Assert::AreEqual("1", response.GetServerRevision().c_str());
                CompareCases(SpanHelpers::CreatePointersSpan(serverCases),
                             SpanHelpers::CreatePointersSpan(ObservableToVector(*response.GetCases())),
                             CompareCasesType::Counts);

                Verify(Method(mockHttp, Request)).Exactly(4);
            }
        }


        TEST_METHOD(TestSyncDataFilePutRetry)
        {
            {
                Mock<HttpConnection> mockHttp;
                When(Method(mockHttp, Request)).
                    AlwaysDo([](HttpRequest) -> HttpResponse{
                    throw SyncRetryableNetworkError(1, "Retryable Error");
                });

                CSWebSyncService sync_service(std::unique_ptr<HttpConnection>(&mockHttp.get()), hostUrl, username_password);

                try {
                    sync_service.PutCases(case_access, SpanHelpers::CreatePointersSpan(clientCases), nullptr, clientDeviceId, universe, std::string());
                    Assert::Fail(L"Expected exception");
                }
                catch (const SyncError &) {

                }
                Verify(Method(mockHttp, Request)).Exactly(MAX_RETRIES + 1);
            }

            {
                Mock<HttpConnection> mockHttp;
                When(Method(mockHttp, Request)).
                    AlwaysDo([](const HttpRequest&) {
                    return HttpResponse(500);
                });

                CSWebSyncService sync_service(std::unique_ptr<HttpConnection>(&mockHttp.get()), hostUrl, username_password);

                try {
                    sync_service.PutCases(case_access, SpanHelpers::CreatePointersSpan(clientCases), nullptr, clientDeviceId, universe, std::string());
                    Assert::Fail(L"Expected exception");
                }
                catch (const SyncError &) {

                }
                Verify(Method(mockHttp, Request)).Exactly(MAX_RETRIES + 1);
            }

            {
                SyncCaseSerializer sync_case_serializer_v2(case_access, SyncCaseSerializer::Version::V2);
                const std::string clientCasesJson = sync_case_serializer_v2.GetSyncableJson(SpanHelpers::CreatePointersSpan(clientCases));

                Mock<HttpConnection> mockHttp;
                When(Method(mockHttp, Request)).Do([](const HttpRequest&) {
                    HttpResponse response(HttpResponse::Status_200_OK);
                    response.body.observable = rxcpp::observable<>::just(std::string("{\"access_token\":\"foo\",\"expires_in\":3600,\"scope\":null,\"token_type\":\"Bearer\",\"refresh_token\":\"foo\"}"));
                    return response;
                }).Do([](const HttpRequest&) {
                    HttpResponse response(HttpResponse::Status_200_OK);
                    response.body.observable = rxcpp::observable<>::just(std::string("{\"deviceId\":\"foo\",\"apiVersion\":2}"));
                    return response;
                }).Do([](const HttpRequest&) {
                    return HttpResponse(500);
                }).Do([](const HttpRequest&) {
                    return HttpResponse(500);
                }).Do([](const HttpRequest&) {
                    return HttpResponse(500);
                }).Do([clientCasesJson](const HttpRequest& request) {
                    std::string postDataJson(std::istreambuf_iterator<char>(*request.upload_data), {});
                    ZLib::Inflate(postDataJson);
                    Assert::AreEqual(clientCasesJson, postDataJson);
                    HttpResponse response(HttpResponse::Status_200_OK);
                    response.headers.Add("Etag", "1");
                    response.body.observable = rxcpp::observable<>::just(std::string("{\"code\":\"200\",\"message\":\"success\"}"));
                    return response;
                });

                CSWebSyncService sync_service(std::unique_ptr<HttpConnection>(&mockHttp.get()), hostUrl, username_password);
                sync_service.Connect();

                const SyncPutResponse response = sync_service.PutCases(case_access, SpanHelpers::CreatePointersSpan(clientCases), nullptr, clientDeviceId, universe, std::string());
                Assert::AreEqual(SyncPutResponse::SyncPutResult::Complete, response.GetResult());
                Assert::AreEqual("1", response.GetServerRevision().c_str());
                Verify(Method(mockHttp, Request)).Exactly(6);
            }
        }


        TEST_METHOD(TestSyncFileGet)
        {
            std::string fileContent = "alkjasjdioajsofijasodifjwioaehfushfijsadiofjasoidjfoiasjdfoisajdfoisajf";

            const std::string destFilePath = Path::Combine(GetTempDirectory(), "synco-get-file-test.txt");
            std::ofstream s(destFilePath, std::ios::binary);
            s << fileContent;
            s.close();

            const std::string expectedMd5 = PortableFunctions::FileMd5(destFilePath);
            PortableFunctions::FileDelete(destFilePath);

            Mock<HttpConnection> mockHttp;

            // Happy path
            When(Method(mockHttp, Request)).
                Do([this, fileContent, expectedMd5](const HttpRequest& request) {
                Assert::AreEqual(hostUrl + "files/remotefile/content", request.url);

                HttpResponse response(HttpResponse::Status_200_OK);
                response.body.observable = rxcpp::observable<>::just(fileContent);
                response.headers.Add_ContentLength(fileContent.length());
                response.headers.Add_ContentMD5(expectedMd5);
                return response;
            });

            CSWebSyncService sync_service(std::unique_ptr<HttpConnection>(&mockHttp.get()), hostUrl, username_password);

            bool result = sync_service.GetFile("/remotefile", destFilePath, std::string());
            Assert::IsTrue(result);
            Verify(Method(mockHttp, Request)).Exactly(1);

            // if-none match: not modified
            const std::string etag = "testetag";
            When(Method(mockHttp, Request)).
                Do([etag](const HttpRequest& request) {
                const std::string ifmatchHeader = request.headers.GetValue("If-None-Match");
                Assert::AreEqual(etag, ifmatchHeader);
                return HttpResponse(304);
            });

            result = sync_service.GetFile("/remotefile", destFilePath, etag);
            Assert::IsFalse(result);

            // http error
            When(Method(mockHttp, Request)).
                Do([](const HttpRequest&) {
                return HttpResponse(500);
            });

            try {
                sync_service.GetFile("/remotefile", destFilePath, std::string());
                Assert::Fail(L"Expected sync error");
            }
            catch (const SyncError&) { }

            // Invalid content md5
            When(Method(mockHttp, Request)).
                Do([fileContent, expectedMd5](const HttpRequest&) {
                HttpResponse response(HttpResponse::Status_200_OK);
                response.body.observable = rxcpp::observable<>::just(fileContent);
                response.headers.Add_ContentLength(fileContent.length());
                response.headers.Add_ContentMD5("BADMD5");
                return response;
            });

            try {
                sync_service.GetFile("/remotefile", destFilePath, std::string());
                Assert::Fail(L"Expected sync error");
            }
            catch (const SyncError& ) {
            }
        }


        TEST_METHOD(TestSyncFilePut)
        {
            std::string fileContent = "alkjasjdioajsofijasodifjwioaehfushfijsadiofjasoidjfoiasjdfoisajdfoisajf";
            int64_t expectedSize = fileContent.length();

            const std::string srcFilePath = Path::Combine(GetTempDirectory(), "synco-get-file-test.txt");
            std::ofstream s(srcFilePath, std::ios::binary);
            s << fileContent;
            s.close();

            const std::string expectedMd5 = PortableFunctions::FileMd5(srcFilePath);

            Mock<HttpConnection> mockHttp;
            CSWebSyncService sync_service(std::unique_ptr<HttpConnection>(&mockHttp.get()), hostUrl, username_password);

            // Happy path
            When(Method(mockHttp, Request)).
                Do([this,fileContent, expectedMd5, expectedSize](const HttpRequest& request) {

                HttpResponse response(HttpResponse::Status_200_OK);
                Assert::AreEqual(hostUrl + "files/remotefile/content", request.url);
                const std::string contentMd5Header = request.headers.GetValue("Content-MD5");
                Assert::AreEqual(expectedMd5, contentMd5Header);

                Assert::AreEqual(expectedSize, request.upload_data_size_bytes);

                std::string postedFileContent(std::istreambuf_iterator<char>(*request.upload_data), {});
                Assert::AreEqual(fileContent, postedFileContent);

                return response;
            });

            sync_service.PutFile(srcFilePath, "/remotefile");
            Verify(Method(mockHttp, Request)).Exactly(1);

            // http error
            When(Method(mockHttp, Request)).
                Do([fileContent, expectedMd5](const HttpRequest&) {
                return HttpResponse(500);
            });

            try {
                sync_service.PutFile(srcFilePath, "/remotefile");
                Assert::Fail(L"Expected sync error");
            }
            catch (const SyncError&) { }
        }


        TEST_METHOD(TestDownloadApplication)
        {
            std::string fileContent = "alkjasjdioajsofijasodifjwioaehfushfijsadiofjasoidjfoiasjdfoisajdfoisajf";

            const std::string destFilePath = Path::Combine(GetTempDirectory(), "zsynco-get-app-test.txt");
            PortableFunctions::FileDelete(destFilePath);

            Mock<HttpConnection> mockHttp;

            // Happy path
            When(Method(mockHttp, Request)).
                Do([this, fileContent](const HttpRequest& request) {
                Assert::AreEqual(hostUrl + "apps/testApp", request.url);
                HttpResponse response(HttpResponse::Status_200_OK);
                response.body.observable = rxcpp::observable<>::just(fileContent);
                response.headers.Add_ContentLength(fileContent.length());
                return response;
            });

            CSWebSyncService sync_service(std::unique_ptr<HttpConnection>(&mockHttp.get()), hostUrl, username_password);

            sync_service.DownloadApplicationPackage("testApp", destFilePath, nullptr, nullptr);
            Verify(Method(mockHttp, Request)).Exactly(1);

            // http error
            When(Method(mockHttp, Request)).
                Do([](const HttpRequest&) {
                return HttpResponse(500);
            });

            try {
                sync_service.DownloadApplicationPackage("testApp", destFilePath, nullptr, nullptr);
                Assert::Fail(L"Expected sync error");
            }
            catch (const SyncError&) { }
        }


        TEST_METHOD(TestSyncDictionaries)
        {
            const std::unique_ptr<const CDataDict> dictionary_with_unique_name = CreateTestDictionaryWithUniqueName();

            const SyncTestCredentials::Credentials credentials = SyncTestCredentials().GetCredentialsCSWeb();
            CSWebSyncServiceWithModifiableApi sync_service(std::make_unique<CurlHttpConnection>(), credentials.sync_connection_string, credentials.username_password);
            sync_service.Connect();

            // test 1: ensure that the dictionary does not already exist
            try
            {
                sync_service.GetCSWebConnection().GetDictionarySpec(dictionary_with_unique_name->GetSyncableName());
                Assert::Fail(L"The dictionary should not already exist");
            }
            catch(...) { }

            // test 2: upload the dictionary without compression
            sync_service.GetCSWebConnection().SetApiVersion(CSWebVersion::V2);
            sync_service.PutDictionary(*dictionary_with_unique_name);

            // test 3: get the dictionary
            const std::string dictionary_text1 = sync_service.GetCSWebConnection().GetDictionarySpec(dictionary_with_unique_name->GetSyncableName());

            // test 4: delete the dictionary
            sync_service.GetCSWebConnection().DeleteDictionarySpec(dictionary_with_unique_name->GetSyncableName());

            // test 5: upload the dictionary with compression
            sync_service.GetCSWebConnection().RestoreApiVersion();
            Assert::IsTrue(DefaultToCSWebApiV2 || sync_service.GetCSWebConnection().GetApiVersion() >= CSWebVersion::V3);
            sync_service.PutDictionary(*dictionary_with_unique_name);

            // test 6: get the dictionary (again)
            const std::string dictionary_text2 = sync_service.GetCSWebConnection().GetDictionarySpec(dictionary_with_unique_name->GetSyncableName());
            Assert::AreEqual(dictionary_text1, dictionary_text2);

            // test 7: validate that the metadata is correct
            if( !DefaultToCSWebApiV2 )
            {
                const JsonNode metadata_node = sync_service.GetCSWebConnection().GetDictionaryMetadata(dictionary_with_unique_name->GetSyncableName());
                Assert::AreEqual(metadata_node.Get<std::string>("dictionaryKeyStructure"), CSWebRepository::CalculateDictionaryKeyStructure(*dictionary_with_unique_name));
                Assert::IsFalse(metadata_node.Contains("minRevision"));
                Assert::IsFalse(metadata_node.Contains("maxRevision"));
            }

            // test 8: delete the dictionary (again)
            sync_service.GetCSWebConnection().DeleteDictionarySpec(dictionary_with_unique_name->GetSyncableName());
        }


        TEST_METHOD(TestSyncFiles)
        {
            const SyncTestCredentials::Credentials credentials = SyncTestCredentials().GetCredentialsCSWeb();
            CSWebSyncServiceWithModifiableApi sync_service(std::make_unique<CurlHttpConnection>(), credentials.sync_connection_string, credentials.username_password);
            sync_service.Connect();

            const std::string input_file_path = Path::Combine(Html::GetDirectory(Html::Subdirectory::Images),
                                                              "report-icon.svg");
            const std::string input_file_md5 = PortableFunctions::FileMd5(input_file_path);

            auto create_remote_path = [&]()
            {
                return FormatText("/test-sync/%s-%d-%s.%s", Path::GetFilenameWithoutExtension(input_file_path).c_str(),
                                                            static_cast<int>(GetTimestamp()), CreateUuid().c_str(),
                                                            PortableFunctions::PathGetFileExtension(input_file_path).c_str());
            };

            // test 1: upload a file without compression
            sync_service.GetCSWebConnection().SetApiVersion(CSWebVersion::V2);
            const std::string remote_path_1 = create_remote_path();
            sync_service.PutFile(input_file_path, remote_path_1);

            // test 2: upload a file with compression
            sync_service.GetCSWebConnection().RestoreApiVersion();
            Assert::IsTrue(DefaultToCSWebApiV2 || sync_service.GetCSWebConnection().GetApiVersion() >= CSWebVersion::V3);
            const std::string remote_path_2 = create_remote_path();
            sync_service.PutFile(input_file_path, remote_path_2);

            // test 3: download the files and validate the MD5s
            TemporaryFile temporary_file_1;
            bool downloaded_file = sync_service.GetFile(remote_path_1, temporary_file_1.GetPath(), std::string());
            Assert::IsTrue(downloaded_file);
            Assert::AreEqual(input_file_md5, PortableFunctions::FileMd5(temporary_file_1.GetPath()));

            TemporaryFile temporary_file_2;
            downloaded_file = sync_service.GetFile(remote_path_2, temporary_file_2.GetPath(), std::string());
            Assert::IsTrue(downloaded_file);
            Assert::AreEqual(input_file_md5, PortableFunctions::FileMd5(temporary_file_2.GetPath()));

            // test 4: make sure the file is not downloaded if if already exists
            downloaded_file = sync_service.GetFile(remote_path_1, temporary_file_1.GetPath(), input_file_md5);
            Assert::IsFalse(downloaded_file);
        }
    };
}
