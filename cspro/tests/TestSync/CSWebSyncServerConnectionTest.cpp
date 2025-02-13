#include "stdafx.h"
#include "CppUnitTest.h"
#include "CaseTestHelpers.h"
#include "TestRepoBuilder.h"
#include <zToolsO/ApiKeys.h>
#include <zToolsO/Utf8Convert.h>
#include <zToolsO/VectorHelpers.h>
#include <zUtilO/ICredentialStore.h>
#include <zNetwork/IHttpConnection.h>
#include <zZipo/ZipUtility.h>
#include <zDictO/DDClass.h>
#include <zSyncO/ConnectResponse.h>
#include <zSyncO/ILoginDialog.h>
#include <zSyncO/CaseObservable.h>
#include <zSyncO/CSWebSyncServerConnection.h>
#include <zSyncO/SyncRequest.h>
#include <zSyncO/JsonConverter.h>
#include <zSyncO/SyncException.h>
#include <zSyncO/CaseJsonWriter.h>
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
    CString random_string(size_t length)
    {
        const wchar_t charset[] =
            L"0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";

        CString str;
        wchar_t* buf = str.GetBufferSetLength(length + 1);
        for (size_t i = 0; i < length; ++i) {
            buf[i] = charset[rand() % (sizeof(charset)/sizeof(charset[0]) - 1)];
        }
        buf[length] = 0;
        str.ReleaseBuffer();
        return str;
    }

    TEST_CLASS(CSWebSyncServerConnectionTest)
    {
    private:
        CString hostUrl = "http://test.com/";
        CString username = "username";
        CString password = "password";
        CString clientDeviceId = "clientDeviceId";
        CString serverDeviceId = "serverDeviceId";
        CString universe = "universe";
        JsonConverter jsonConverter;

        std::unique_ptr<const CDataDict> dictionary = createTestDict();
        std::shared_ptr<CaseAccess> case_access = CaseAccess::CreateAndInitializeFullCaseAccess(*dictionary);
        const CString dictName = dictionary->GetName();

        CString createNumberedCaseGuid(int n)
        {
            CString id;
            id.Format(L"guid%06d", n);
            return id;
        }

        std::vector<std::shared_ptr<Case>> createClientTestCases()
        {
            VectorClock clientCaseClock;
            clientCaseClock.increment(clientDeviceId);
            auto clientCase1 = makeCase(*case_access, L"guidc1", 1, { L"clientdata1" }, false);
            clientCase1->SetVectorClock(clientCaseClock);
            auto clientCase2 = makeCase(*case_access, L"guidc2", 2, { L"clientdata2" }, false);
            clientCase2->SetVectorClock(clientCaseClock);
            return { clientCase1, clientCase2 };
        }

        std::vector<std::shared_ptr<Case>> createServerTestCases()
        {
            VectorClock serverCaseClock;
            serverCaseClock.increment(serverDeviceId);
            auto serverCase1 = makeCase(*case_access, L"guids1", 1, { L"serverdata1" }, false);
            serverCase1->SetVectorClock(serverCaseClock);
            auto serverCase2 = makeCase(*case_access, L"guids2", 2, { L"serverdata2" }, false);
            serverCase2->SetVectorClock(serverCaseClock);
            return { serverCase1, serverCase2 };

        }

        std::vector<std::shared_ptr<Case>> serverCases = createServerTestCases();

        std::vector<std::shared_ptr<Case>> clientCases = createClientTestCases();

        const int MAX_RETRIES = 3;

        CString stripQuotes(CString s)
        {
            if (s.IsEmpty())
                return s;

            if (s[0] == _T('\"'))
                s = s.Mid(1);
            if (s[s.GetLength() - 1] == _T('\"'))
                s = s.Left(s.GetLength() - 1);

            return s;
        }

    public:

        TEST_METHOD(TestSyncDataFileGet)
        {
            Mock<IHttpConnection> mockHttp;

            auto serverCasesJson = convertCasesToJSON(jsonConverter, serverCases);

            CString serverRevision = "1";

            When(Method(mockHttp, Request)).
                Do([serverCasesJson, serverRevision, this](const HttpRequest& request) {
                CString expectedUrl = hostUrl + "dictionaries/" + dictName + "/cases";
                Assert::AreEqual(expectedUrl, request.url, L"URL does not match");
                Assert::AreEqual(clientDeviceId, request.headers.value(L"x-csw-device"));
                Assert::AreEqual(universe, stripQuotes(request.headers.value(L"x-csw-universe")));
                Assert::AreEqual(CString(), request.headers.value(L"x-csw-if-revision-exists"));
                HttpResponse response(200);
                response.headers.push_back(CString(L"etag: ") + serverRevision);
                response.body.observable = rxcpp::observable<>::just(serverCasesJson);
                return response;
            });

            auto mockHttpConnectionPtr = std::shared_ptr<IHttpConnection>(&mockHttp.get(), [](...) {});
            CSWebSyncServerConnection connection(mockHttpConnectionPtr, hostUrl, username, password);

            SyncRequest request(*case_access, clientDeviceId, universe, CString(), CString(), std::vector<CString>());
            SyncGetResponse response = connection.getData(request);
            Assert::AreEqual(SyncGetResponse::SyncGetResult::Complete, response.getResult());
            Assert::AreEqual(serverRevision, response.getServerRevision());
            checkCaseList(serverCases, ObservableToVector(*response.getCases()), false);
            Verify(Method(mockHttp, Request)).Once();
        }

        TEST_METHOD(TestSyncDataFileGetBadJson)
        {
            Mock<IHttpConnection> mockHttp;

            When(Method(mockHttp, Request)).
                Do([](const HttpRequest&) {
                HttpResponse response(200);
                response.headers.push_back(CString(L"etag: 1"));
                response.body.observable = rxcpp::observable<>::just(std::string("some totally invalid json"));
                return response;
            });

            auto mockHttpConnectionPtr = std::shared_ptr<IHttpConnection>(&mockHttp.get(), [](...) {});
            CSWebSyncServerConnection connection(mockHttpConnectionPtr, hostUrl, username, password);

            SyncRequest request(*case_access, clientDeviceId, universe, CString(), CString(), std::vector<CString>());
            SyncGetResponse response = connection.getData(request);
            Assert::ExpectException<SyncError>([this, &response]() { checkCaseList(serverCases, ObservableToVector(*response.getCases()), false); });
        }

        TEST_METHOD(TestSyncDataFilePut)
        {
            Mock<IHttpConnection> mockHttp;

            CaseJsonWriter json_writer;
            json_writer.SetStringifyData(true);
            auto clientCasesJson = json_writer.ToJson(clientCases);

            CString serverRevision = "1";

            When(Method(mockHttp, Request)).
                Do([](const HttpRequest&) {
                HttpResponse response(200);
                response.body.observable = rxcpp::observable<>::just(std::string("{\"access_token\":\"foo\",\"expires_in\":3600,\"scope\":null,\"token_type\":\"Bearer\",\"refresh_token\":\"foo\"}"));
                return response;
            }).Do([](const HttpRequest&) {
                HttpResponse response(200);
                response.body.observable = rxcpp::observable<>::just(std::string("{\"deviceId\":\"foo\",\"apiVersion\":2}"));
                return response;
            }).Do([clientCasesJson, serverRevision, this](const HttpRequest& request) {
                CString expectedUrl = hostUrl + "dictionaries/" + dictName + "/cases";
                Assert::AreEqual(expectedUrl, request.url, L"URL does not match");
                Assert::AreEqual(clientDeviceId, request.headers.value(L"x-csw-device"));
                Assert::AreEqual(CString(), request.headers.value(L"x-csw-universe"));
                Assert::AreEqual(CString(), request.headers.value(L"x-csw-if-revision-exists"));
                std::string postDataJson(std::istreambuf_iterator<char>(*request.upload_data), {});
                ZipUtility::decompression(postDataJson);
                Assert::AreEqual(clientCasesJson, postDataJson);
                HttpResponse response(200);
                response.headers.push_back(CString(L"etag: ") + serverRevision);
                response.body.observable = rxcpp::observable<>::just(std::string("{\"code\":\"200\",\"message\":\"success\"}"));
                return response;
            });

            auto mockHttpConnectionPtr = std::shared_ptr<IHttpConnection>(&mockHttp.get(), [](...) {});
            CSWebSyncServerConnection connection(mockHttpConnectionPtr, hostUrl, username, password);
            connection.connect();

            SyncRequest request(*case_access, clientDeviceId, universe, clientCases, CString());
            auto response = connection.putData(request);
            Assert::AreEqual(SyncPutResponse::SyncPutResult::Complete, response.getResult());
            Assert::AreEqual(serverRevision, response.getServerRevision());

            Verify(Method(mockHttp, Request)).Exactly(3);
        }

        TEST_METHOD(TestSyncDataFileHttpError)
        {
            Mock<IHttpConnection> mockHttp;

            auto clientCasesJson = convertCasesToJSON(jsonConverter, clientCases);

            CString getServerRevision = "1";

            When(Method(mockHttp, Request)).
                Do([](const HttpRequest&) {
                HttpResponse response(404);
                response.body.observable = rxcpp::observable<>::just(std::string("{code=404, message=\"Not found\"}"));
                return response;
            });

            auto mockHttpConnectionPtr = std::shared_ptr<IHttpConnection>(&mockHttp.get(), [](...) {});
            CSWebSyncServerConnection connection(mockHttpConnectionPtr, hostUrl, username, password);
            SyncRequest request(*case_access, clientDeviceId, universe, clientCases, CString());
            try {
                connection.putData(request);
                Assert::Fail(L"Expected exception");
            }
            catch (const SyncError&) {
            }

            Verify(Method(mockHttp, Request)).Once();
        }

        TEST_METHOD(TestSyncDataFileGetChunked)
        {
            Mock<IHttpConnection> mockHttp;

            int totalCases = -1;
            int lastCaseNumSent = -1;
            int revisionNum = 0;
            std::vector<std::shared_ptr<Case>> allCases;
            std::vector<std::shared_ptr<Case>> responseCases;

            When(Method(mockHttp, Request)).
                AlwaysDo([this, &totalCases, &lastCaseNumSent, &revisionNum, &allCases](const HttpRequest& request) {
                CString startGuid = request.headers.value(L"x-csw-case-range-start-after");
                if (lastCaseNumSent == -1) {
                    // First time should be no start after
                    Assert::AreEqual(CString(), startGuid);
                } else {
                    Assert::AreEqual(createNumberedCaseGuid(lastCaseNumSent), startGuid);
                }
                int startCaseNum = lastCaseNumSent + 1;
                CString rangeCount = request.headers.value(L"x-csw-case-range-count");
                Assert::AreNotEqual(CString(), rangeCount);
                int numCasesToReturn = _wtoi(rangeCount);
                if (totalCases == -1)
                    totalCases = int(numCasesToReturn * 3.5); // so that we get 3 calls total with chunk 1: n, chunk 2: 2n, chunk 3: 0.5n 

                if (lastCaseNumSent + numCasesToReturn > totalCases) {
                    numCasesToReturn = totalCases - startCaseNum;
                }
                VectorClock serverCaseClock;
                serverCaseClock.increment(serverDeviceId);
                std::vector<std::shared_ptr<Case>> casesThisChunk;
                for (int i = 0; i < numCasesToReturn; ++i) {
                    auto data_case = makeCase(*case_access, createNumberedCaseGuid(i + startCaseNum), i, { L"serverdata" }, false);
                    data_case->SetVectorClock(serverCaseClock);
                    casesThisChunk.push_back(data_case);
                }
                std::copy(casesThisChunk.begin(), casesThisChunk.end(), std::back_inserter(allCases));

                lastCaseNumSent += numCasesToReturn;
                HttpResponse response(lastCaseNumSent < totalCases - 1 ? 206 : 200);
                auto casesJson = convertCasesToJSON(jsonConverter, casesThisChunk);
                response.body.observable = rxcpp::observable<>::just(casesJson);

                CString rangeCountResponse;
                rangeCountResponse.Format(L"x-csw-case-range-count: %d/%d", numCasesToReturn, totalCases);
                response.headers.push_back(rangeCountResponse);

                CString etagResponse;
                etagResponse.Format(L"etag: %d", ++revisionNum); // increase rev number to simulate other devices to syncing to server in between requests
                response.headers.push_back(etagResponse);

                return response;
            });

            auto mockHttpConnectionPtr = std::shared_ptr<IHttpConnection>(&mockHttp.get(), [](...) {});
            CSWebSyncServerConnection connection(mockHttpConnectionPtr, hostUrl, username, password);

            auto response = connection.getData(SyncRequest(*case_access, clientDeviceId, universe, CString(), CString(), std::vector<CString>()));
            Assert::AreEqual(SyncGetResponse::SyncGetResult::MoreData, response.getResult());
            Assert::AreEqual(revisionNum, _wtoi(response.getServerRevision()));
            VectorHelpers::Append(responseCases, ObservableToVector(*response.getCases()));

            response = connection.getData(SyncRequest(*case_access, clientDeviceId, universe, response.getServerRevision(), responseCases.back()->GetUuid(), std::vector<CString>()));
            Assert::AreEqual(SyncGetResponse::SyncGetResult::MoreData, response.getResult());
            Assert::AreEqual(revisionNum, _wtoi(response.getServerRevision()));
            VectorHelpers::Append(responseCases, ObservableToVector(*response.getCases()));

            SyncRequest request3(*case_access, clientDeviceId, universe, response.getServerRevision(), responseCases.back()->GetUuid(), std::vector<CString>());
            response = connection.getData(request3);
            Assert::AreEqual(SyncGetResponse::SyncGetResult::Complete, response.getResult());
            Assert::AreEqual(revisionNum, _wtoi(response.getServerRevision()));
            VectorHelpers::Append(responseCases, ObservableToVector(*response.getCases()));

            checkCaseList(allCases, responseCases, false);
            Verify(Method(mockHttp, Request)).Exactly(3);
        }
        
        TEST_METHOD(TestSyncDataFileGetAdaptiveChunkSize)
        {
            Mock<IHttpConnection> mockHttp;

            const int initialChunkSize = 100;
            int totalCases = initialChunkSize * 10;
            int nextCaseNum = 1;
            int requests = 0;

            When(Method(mockHttp, Request)).
                AlwaysDo([this, initialChunkSize, &requests, &totalCases, &nextCaseNum](const HttpRequest& request) {

                CString rangeCount = request.headers.value(L"x-csw-case-range-count");
                Assert::AreNotEqual(CString(), rangeCount);
                int chunkSizeRequested = _wtoi(rangeCount);
                size_t caseDataSize = 100;

                ++requests;

                if (requests == 1) {
                    Assert::AreEqual(initialChunkSize, chunkSizeRequested);
                } else if (requests == 2) {
                    Assert::AreEqual(initialChunkSize * 2, chunkSizeRequested); // chunk size should have doubled
                    Sleep(30010);
                }
                else if (requests == 3) {
                    // Should still be 200 since last request took more than 30 secs
                    Assert::AreEqual(initialChunkSize*2, chunkSizeRequested);
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
                    std::vector<CString> caseData;
                    do {
                        caseData.emplace_back(random_string(30));
                    } while (caseData.size() * 30 < caseDataSize);

                    auto data_case = makeCase(*case_access, createNumberedCaseGuid(nextCaseNum), i, caseData, false);
                    data_case->SetVectorClock(serverCaseClock);
                    casesThisChunk.push_back(data_case);
                }

                HttpResponse response(requests < 5 ? 206 : 200);

                auto casesJson = convertCasesToJSON(jsonConverter, casesThisChunk);
                response.body.observable = rxcpp::observable<>::just(casesJson);

                CString rangeCountResponse;
                rangeCountResponse.Format(L"x-csw-case-range-count: %d/%d", chunkSizeRequested, totalCases);
                response.headers.push_back(rangeCountResponse);

                CString etagResponse(L"etag: 1");
                response.headers.push_back(etagResponse);

                return response;
            });

            auto mockHttpConnectionPtr = std::shared_ptr<IHttpConnection>(&mockHttp.get(), [](...) {});
            CSWebSyncServerConnection connection(mockHttpConnectionPtr, hostUrl, username, password);

            SyncGetResponse::SyncGetResult result;

            do {
                auto response = connection.getData(SyncRequest(*case_access, clientDeviceId, universe, CString(), CString(), std::vector<CString>()));
                ObservableToVector(*response.getCases());
                result = response.getResult();
            } while (result == SyncGetResponse::SyncGetResult::MoreData);

        }

        TEST_METHOD(TestSyncDataFileGetAfterServerReset)
        {
            Mock<IHttpConnection> mockHttp;
            CString lastServerRevision = "5";

            When(Method(mockHttp, Request)).
                Do([lastServerRevision, this](const HttpRequest& request) {
                CString expectedUrl = hostUrl + "dictionaries/" + dictName + "/cases";
                Assert::AreEqual(expectedUrl, request.url, L"URL does not match");
                Assert::AreEqual(clientDeviceId, request.headers.value(L"x-csw-device"));
                Assert::AreEqual(universe, stripQuotes(request.headers.value(L"x-csw-universe")));
                Assert::AreEqual(lastServerRevision, request.headers.value(L"x-csw-if-revision-exists"));
                return HttpResponse(412);
            });

            auto mockHttpConnectionPtr = std::shared_ptr<IHttpConnection>(&mockHttp.get(), [](...) {});
            CSWebSyncServerConnection connection(mockHttpConnectionPtr, hostUrl, username, password);

            SyncRequest request(*case_access, clientDeviceId, universe, lastServerRevision, CString(), std::vector<CString>());
            auto response = connection.getData(request);
            Assert::AreEqual(SyncGetResponse::SyncGetResult::RevisionNotFound, response.getResult());
            Verify(Method(mockHttp, Request)).Once();
        }

        TEST_METHOD(TestSyncDataFilePutAfterServerReset)
        {
            Mock<IHttpConnection> mockHttp;

            CString lastServerRevision = "5";

            When(Method(mockHttp, Request)).
                Do([lastServerRevision, this](const HttpRequest& request) {
                CString expectedUrl = hostUrl + "dictionaries/" + dictName + "/cases";
                Assert::AreEqual(expectedUrl, request.url, L"URL does not match");
                Assert::AreEqual(clientDeviceId, request.headers.value(L"x-csw-device"));
                Assert::AreEqual(CString(), request.headers.value(L"x-csw-universe"));
                Assert::AreEqual(lastServerRevision, request.headers.value(L"x-csw-if-revision-exists"));
                return HttpResponse(412);
            });

            auto mockHttpConnectionPtr = std::shared_ptr<IHttpConnection>(&mockHttp.get(), [](...) {});
            CSWebSyncServerConnection connection(mockHttpConnectionPtr, hostUrl, username, password);

            SyncRequest request(*case_access, clientDeviceId, universe, clientCases, lastServerRevision);
            auto response = connection.putData(request);
            Assert::AreEqual(SyncPutResponse::SyncPutResult::RevisionNotFound, response.getResult());
            Verify(Method(mockHttp, Request)).Once();
        }

        TEST_METHOD(TestSyncDataFileRefreshAuthToken)
        {
            Mock<IHttpConnection> mockHttp;

            CaseJsonWriter json_writer;
            json_writer.SetStringifyData(true);
            auto clientCasesJson = json_writer.ToJson(clientCases);
            auto serverCasesJson = json_writer.ToJson(serverCases);

            CString getServerRevision = "1";
            CString postServerRevision = "2";

            CString accessToken1 = L"accessToken1";
            CString refreshToken1 = L"refreshToken1";
            CString accessToken2 = L"accessToken2";
            CString refreshToken2 = L"refreshToken2";
            CString accessToken3 = L"accessToken3";
            CString refreshToken3 = L"refreshToken3";

            When(Method(mockHttp, Request)).
                Do([accessToken1, refreshToken1, this](const HttpRequest& request) {
                // First post will be to token endpoint
                Assert::IsTrue(request.method == HttpRequestMethod::HTTP_POST);
                CString expectedUrl = hostUrl + "token";
                Assert::AreEqual(expectedUrl, request.url, L"URL does not match");
                CString expectedTokenRequest;
                expectedTokenRequest.Format(L"{\"client_id\":\"%s\",\"client_secret\":\"%s\",\"grant_type\":\"password\",\"username\":\"%s\",\"password\":\"%s\"}",
                    CSWebKeys::client_id, CSWebKeys::client_secret, (LPCTSTR)username, (LPCTSTR)password);
                std::string postDataJson(std::istreambuf_iterator<char>(*request.upload_data), {});
                Assert::AreEqual(expectedTokenRequest, UTF8Convert::UTF8ToWide<CString>(postDataJson));
                HttpResponse response(200);
                std::ostringstream oss;
                oss << "{\"access_token\":\"" << UTF8Convert::WideToUTF8(accessToken1) << "\",\"expires_in\":3600,\"token_type\":\"Bearer\",\"scope\":null,\"refresh_token\":\"" << UTF8Convert::WideToUTF8(refreshToken1) << "\"}";
                response.body.observable = rxcpp::observable<>::just(oss.str());
                return response;
             }).
                Do([accessToken1, this](const HttpRequest& request) {
                // First get is get server info
                Assert::IsTrue(request.method == HttpRequestMethod::HTTP_GET);
                CString expectedUrl = hostUrl + "server";
                Assert::AreEqual(expectedUrl, request.url, L"URL does not match");
                Assert::AreEqual(CString(L"Bearer ") + accessToken1, request.headers.value(L"Authorization"));
                HttpResponse response(200);
                std::ostringstream oss;
                oss << "{\"deviceId\":\"" << UTF8Convert::WideToUTF8(serverDeviceId) << "\", \"apiVersion\":2.0}";
                response.body.observable = rxcpp::observable<>::just(oss.str());
                return response;
            }).
                Do([accessToken1, this](const HttpRequest& request) {
                // Simulate expired token, force refresh
                Assert::IsTrue(request.method == HttpRequestMethod::HTTP_GET);
                CString expectedUrl = hostUrl + "dictionaries/" + dictName + "/cases";
                Assert::AreEqual(expectedUrl, request.url, L"URL does not match");
                Assert::AreEqual(CString(L"Bearer ") + accessToken1, request.headers.value(L"Authorization"));
                return HttpResponse(401);
            }).
                Do([refreshToken1, accessToken2, refreshToken2](const HttpRequest& request) {
                    // Second post is refresh token
                    Assert::IsTrue(request.method == HttpRequestMethod::HTTP_POST);
                    CString expectedTokenRequest;
                    expectedTokenRequest.Format(L"{\"client_id\":\"%s\",\"client_secret\":\"%s\",\"grant_type\":\"refresh_token\",\"refresh_token\":\"%s\"}",
                        CSWebKeys::client_id, CSWebKeys::client_secret, (LPCTSTR)refreshToken1);
                    std::string postDataJson(std::istreambuf_iterator<char>(*request.upload_data), {});
                    Assert::AreEqual(expectedTokenRequest, UTF8Convert::UTF8ToWide<CString>(postDataJson));
                    HttpResponse response(200);
                    std::ostringstream oss;
                    oss << "{\"access_token\":\"" << UTF8Convert::WideToUTF8(accessToken2) << "\",\"expires_in\":3600,\"token_type\":\"Bearer\",\"scope\":null,\"refresh_token\":\"" << UTF8Convert::WideToUTF8(refreshToken2) << "\"}";
                    response.body.observable = rxcpp::observable<>::just(oss.str());
                    return response;
             }).
                Do([accessToken2, serverCasesJson, getServerRevision, this](const HttpRequest& request) {
                 // get with the refresh token
                Assert::IsTrue(request.method == HttpRequestMethod::HTTP_GET);
                CString expectedUrl = hostUrl + "dictionaries/" + dictName + "/cases";
                Assert::AreEqual(expectedUrl, request.url, L"URL does not match");
                Assert::AreEqual(CString(L"Bearer ") + accessToken2, request.headers.value(L"Authorization"));
                Assert::AreEqual(clientDeviceId, request.headers.value(L"x-csw-device"));
                Assert::AreEqual(universe, stripQuotes(request.headers.value(L"x-csw-universe")));
                Assert::AreEqual(CString(), request.headers.value(L"x-csw-if-revision-exists"));
                HttpResponse response(200);
                response.headers.push_back(CString(L"etag: ") + getServerRevision);
                response.body.observable = rxcpp::observable<>::just(serverCasesJson);
                return response;
            }).
               Do([accessToken2, this](const HttpRequest& request) {
                // Third post is case upload
                Assert::IsTrue(request.method == HttpRequestMethod::HTTP_POST);
                CString expectedUrl = hostUrl + "dictionaries/" + dictName + "/cases";
                Assert::AreEqual(expectedUrl, request.url);
                Assert::AreEqual(CString(L"Bearer ") + accessToken2, request.headers.value(L"Authorization"));
                // Simulate expired token, force refresh
                return HttpResponse(401);
            }).Do([refreshToken2, accessToken3, refreshToken3](const HttpRequest& request) {
                // Fourth post is refresh token again
                Assert::IsTrue(request.method == HttpRequestMethod::HTTP_POST);
                CString expectedTokenRequest;
                expectedTokenRequest.Format(L"{\"client_id\":\"%s\",\"client_secret\":\"%s\",\"grant_type\":\"refresh_token\",\"refresh_token\":\"%s\"}",
                    CSWebKeys::client_id, CSWebKeys::client_secret, (LPCTSTR)refreshToken2);
                std::string postDataJson(std::istreambuf_iterator<char>(*request.upload_data), {});
                Assert::AreEqual(expectedTokenRequest, UTF8Convert::UTF8ToWide<CString>(postDataJson));
                HttpResponse response(200);
                std::ostringstream oss;
                oss << "{\"access_token\":\"" << UTF8Convert::WideToUTF8(accessToken3) << "\",\"expires_in\":3600,\"token_type\":\"Bearer\",\"scope\":null,\"refresh_token\":\"" << UTF8Convert::WideToUTF8(refreshToken3) << "\"}";
                response.body.observable = rxcpp::observable<>::just(oss.str());
                return response;
            }).Do([accessToken3, clientCasesJson, postServerRevision, this](const HttpRequest& request) {
                // Fifth post is case upload again after getting refresh token
                Assert::IsTrue(request.method == HttpRequestMethod::HTTP_POST);
                CString expectedUrl = hostUrl + "dictionaries/" + dictName + "/cases";
                Assert::AreEqual(expectedUrl, request.url, L"URL does not match");
                Assert::AreEqual(CString(L"Bearer ") + accessToken3, request.headers.value(L"Authorization"));
                Assert::AreEqual(clientDeviceId, request.headers.value(L"x-csw-device"));
                Assert::AreEqual(CString(), request.headers.value(L"x-csw-universe"));
                Assert::AreEqual(CString(), request.headers.value(L"x-csw-if-revision-exists"));
                std::string postDataJson(std::istreambuf_iterator<char>(*request.upload_data), {});
                ZipUtility::decompression(postDataJson);
                Assert::AreEqual(clientCasesJson, postDataJson);
                HttpResponse response(200);
                response.headers.push_back(CString(L"etag: ") + postServerRevision);
                response.body.observable = rxcpp::observable<>::just(std::string("{\"code\":\"200\",\"message\":\"success\"}"));
                return response;
                });

            Mock<ILoginDialog> mockLogin;
            When(Method(mockLogin, Show)).AlwaysDo([this](const CString&, bool) -> std::optional<std::tuple<CString, CString>> {return std::make_tuple(username, password); });

            Mock<ICredentialStore> mockCredentialStore;
            When(Method(mockCredentialStore, Store)).AlwaysReturn();
            When(Method(mockCredentialStore, Retrieve)).AlwaysReturn(L"");

            auto mockHttpConnectionPtr = std::shared_ptr<IHttpConnection>(&mockHttp.get(), [](...) {});
            CSWebSyncServerConnection connection(mockHttpConnectionPtr, hostUrl, &mockLogin.get(), &mockCredentialStore.get());

            std::unique_ptr<ConnectResponse> pConnectResponse(connection.connect());
            Assert::AreEqual(serverDeviceId, pConnectResponse->getServerDeviceId());

            auto getResponse = connection.getData(SyncRequest(*case_access, clientDeviceId, universe, CString(), CString(), std::vector<CString>()));
            Assert::AreEqual(SyncGetResponse::SyncGetResult::Complete, getResponse.getResult());
            Assert::AreEqual(getServerRevision, getResponse.getServerRevision());
            checkCaseList(serverCases, ObservableToVector(*getResponse.getCases()), false);

            auto putResponse = connection.putData(SyncRequest(*case_access, clientDeviceId, universe, clientCases, CString()));
            Assert::AreEqual(SyncPutResponse::SyncPutResult::Complete, putResponse.getResult());
            Assert::AreEqual(postServerRevision, putResponse.getServerRevision());

            Verify(Method(mockHttp, Request)).Exactly(8);
        }

        TEST_METHOD(TestSyncDataFileGetRetry)
        {

            {
                Mock<IHttpConnection> mockHttp;
                When(Method(mockHttp, Request)).
                    AlwaysDo([](const HttpRequest&) {
                    throw SyncRetryableNetworkError(1, L"Retryable Error");
                    return HttpResponse(200);
                });

                auto mockHttpConnectionPtr = std::shared_ptr<IHttpConnection>(&mockHttp.get(), [](...) {});
                CSWebSyncServerConnection connection(mockHttpConnectionPtr, hostUrl, username, password);

                try {
                    connection.getData(SyncRequest(*case_access, clientDeviceId, universe, CString(), CString(), std::vector<CString>()));
                    Assert::Fail(L"Should have gotten an exception");
                }
                catch (const SyncError&) {

                }
                Verify(Method(mockHttp, Request)).Exactly(MAX_RETRIES + 1);
            }

            {
                Mock<IHttpConnection> mockHttp;
                When(Method(mockHttp, Request)).
                    AlwaysDo([this](const HttpRequest&) {
                    return HttpResponse(500);
                });

                auto mockHttpConnectionPtr = std::shared_ptr<IHttpConnection>(&mockHttp.get(), [](...) {});
                CSWebSyncServerConnection connection(mockHttpConnectionPtr, hostUrl, username, password);

                try {
                    connection.getData(SyncRequest(*case_access, clientDeviceId, universe, CString(), CString(), std::vector<CString>()));
                    Assert::Fail(L"Should have gotten an exception");
                }
                catch (const SyncError&) {

                }
                Verify(Method(mockHttp, Request)).Exactly(MAX_RETRIES + 1);
            }

            auto serverCasesJson = convertCasesToJSON(jsonConverter, serverCases);
            {
                Mock<IHttpConnection> mockHttp;
                When(Method(mockHttp, Request)).
                    Do([](const HttpRequest&) {
                    return HttpResponse(500);
                }).Do([](const HttpRequest&) {
                    return HttpResponse(500);
                }).Do([](const HttpRequest&) {
                        return HttpResponse(500);
                }).Do([serverCasesJson](const HttpRequest&) {
                    HttpResponse response(200);
                    response.headers.push_back(CString(L"etag: 1"));
                    response.body.observable = rxcpp::observable<>::just(serverCasesJson);
                    return response;
                });

                auto mockHttpConnectionPtr = std::shared_ptr<IHttpConnection>(&mockHttp.get(), [](...) {});
                CSWebSyncServerConnection connection(mockHttpConnectionPtr, hostUrl, username, password);

                auto response = connection.getData(SyncRequest(*case_access, clientDeviceId, universe, CString(), CString(), std::vector<CString>()));
                Assert::AreEqual(SyncGetResponse::SyncGetResult::Complete, response.getResult());
                Assert::AreEqual(L"1", response.getServerRevision());
                checkCaseList(serverCases, ObservableToVector(*response.getCases()),false);
                Verify(Method(mockHttp, Request)).Exactly(4);
            }
        }

        TEST_METHOD(TestSyncDataFilePutRetry)
        {

            {
                Mock<IHttpConnection> mockHttp;
                When(Method(mockHttp, Request)).
                    AlwaysDo([](const HttpRequest&) {
                    throw SyncRetryableNetworkError(1, L"Retryable Error");
                    return HttpResponse(200);
                });

                auto mockHttpConnectionPtr = std::shared_ptr<IHttpConnection>(&mockHttp.get(), [](...) {});
                CSWebSyncServerConnection connection(mockHttpConnectionPtr, hostUrl, username, password);

                try {
                    connection.putData(SyncRequest(*case_access, clientDeviceId, universe, clientCases, CString()));
                    Assert::Fail(L"Expected exception");
                }
                catch (const SyncError &) {

                }
                Verify(Method(mockHttp, Request)).Exactly(MAX_RETRIES + 1);
            }

            {
                Mock<IHttpConnection> mockHttp;
                When(Method(mockHttp, Request)).
                    AlwaysDo([](const HttpRequest&) {
                    return HttpResponse(500);
                });

                auto mockHttpConnectionPtr = std::shared_ptr<IHttpConnection>(&mockHttp.get(), [](...) {});
                CSWebSyncServerConnection connection(mockHttpConnectionPtr, hostUrl, username, password);

                try {
                    connection.putData(SyncRequest(*case_access, clientDeviceId, universe, clientCases, CString()));
                    Assert::Fail(L"Expected exception");
                }
                catch (const SyncError &) {

                }
                Verify(Method(mockHttp, Request)).Exactly(MAX_RETRIES + 1);
            }

            {
                CaseJsonWriter json_writer;
                json_writer.SetStringifyData(true);
                auto clientCasesJson = json_writer.ToJson(clientCases);

                Mock<IHttpConnection> mockHttp;
                When(Method(mockHttp, Request)).Do([](const HttpRequest&) {
                    HttpResponse response(200);
                    response.body.observable = rxcpp::observable<>::just(std::string("{\"access_token\":\"foo\",\"expires_in\":3600,\"scope\":null,\"token_type\":\"Bearer\",\"refresh_token\":\"foo\"}"));
                    return response;
                }).Do([](const HttpRequest&) {
                    HttpResponse response(200);
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
                    ZipUtility::decompression(postDataJson);
                    Assert::AreEqual(clientCasesJson, postDataJson);
                    HttpResponse response(200);
                    response.headers.push_back(CString(L"etag: 1"));
                    response.body.observable = rxcpp::observable<>::just(std::string("{\"code\":\"200\",\"message\":\"success\"}"));
                    return response;
                });

                auto mockHttpConnectionPtr = std::shared_ptr<IHttpConnection>(&mockHttp.get(), [](...) {});
                CSWebSyncServerConnection connection(mockHttpConnectionPtr, hostUrl, username, password);
                connection.connect();
                auto response = connection.putData(SyncRequest(*case_access, clientDeviceId, universe, clientCases, CString()));
                Assert::AreEqual(SyncPutResponse::SyncPutResult::Complete, response.getResult());
                Assert::AreEqual(L"1", response.getServerRevision());
                Verify(Method(mockHttp, Request)).Exactly(6);
            }

        }

        TEST_METHOD(TestSyncFileGet)
        {
            std::string fileContent = "alkjasjdioajsofijasodifjwioaehfushfijsadiofjasoidjfoiasjdfoisajdfoisajf";

            wchar_t pszTempPath[MAX_PATH];
            GetTempPath(MAX_PATH, pszTempPath);
            CString destFilePath;
            destFilePath.Format(L"%szsynco-get-file-test.txt", pszTempPath);
            std::ofstream s(destFilePath, std::ios::binary);
            s << fileContent;
            s.close();

            std::wstring expectedMd5 = PortableFunctions::FileMd5(destFilePath);
            DeleteFile(destFilePath);

            Mock<IHttpConnection> mockHttp;

            // Happy path
            When(Method(mockHttp, Request)).
                Do([this, fileContent, expectedMd5](const HttpRequest& request) {
                Assert::AreEqual(hostUrl + L"files/remotefile/content", request.url);

                HttpResponse response(200);
                response.body.observable = rxcpp::observable<>::just(fileContent);
                CString lengthHeader;
                lengthHeader.Format(L"Content-Length: %d", fileContent.length());
                response.headers.push_back(lengthHeader);
                CString md5Header;
                md5Header.Format(L"Content-MD5: %s", expectedMd5.c_str());
                response.headers.push_back(md5Header);
                return response;
            });

            auto mockHttpConnectionPtr = std::shared_ptr<IHttpConnection>(&mockHttp.get(), [](...) {});
            CSWebSyncServerConnection connection(mockHttpConnectionPtr, hostUrl, username, password);

            bool result = connection.getFile(L"/remotefile", destFilePath, destFilePath);
            Assert::IsTrue(result);
            Verify(Method(mockHttp, Request)).Exactly(1);

            // if-none match: not modified
            CString etag = L"testetag";
            When(Method(mockHttp, Request)).
                Do([etag](const HttpRequest& request) {
                CString ifmatchHeader = request.headers.value(L"If-None-Match");
                Assert::AreEqual(etag, ifmatchHeader);
                return HttpResponse(304);
            });

            result = connection.getFile(L"/remotefile", destFilePath, destFilePath, etag);
            Assert::IsFalse(result);

            // http error
            When(Method(mockHttp, Request)).
                Do([](const HttpRequest&) {
                return HttpResponse(500);
            });

            try {
                connection.getFile(L"/remotefile", destFilePath, destFilePath);
                Assert::Fail(L"Expected sync error");
            }
            catch (const SyncError&) {
            }

            // Invalid content md5
            When(Method(mockHttp, Request)).
                Do([fileContent, expectedMd5](const HttpRequest&) {
                HttpResponse response(200);
                response.body.observable = rxcpp::observable<>::just(fileContent);
                CString lengthHeader;
                lengthHeader.Format(L"Content-Length: %d", fileContent.length());
                response.headers.push_back(lengthHeader);
                CString md5Header;
                md5Header.Format(L"Content-MD5: %s", L"BADMD5");
                response.headers.push_back(md5Header);
                return response;
            });

            try {
                connection.getFile(L"/remotefile", destFilePath, destFilePath);
                Assert::Fail(L"Expected sync error");
            }
            catch (const SyncError& ) {
            }

        }

        TEST_METHOD(TestSyncFilePut)
        {
            std::string fileContent = "alkjasjdioajsofijasodifjwioaehfushfijsadiofjasoidjfoiasjdfoisajdfoisajf";
            int64_t expectedSize = fileContent.length();

            wchar_t pszTempPath[MAX_PATH];
            GetTempPath(MAX_PATH, pszTempPath);
            CString srcFilePath;
            srcFilePath.Format(L"%szsynco-get-file-test.txt", pszTempPath);
            std::ofstream s(srcFilePath, std::ios::binary);
            s << fileContent;
            s.close();

            std::wstring expectedMd5 = PortableFunctions::FileMd5(srcFilePath);

            Mock<IHttpConnection> mockHttp;
            auto mockHttpConnectionPtr = std::shared_ptr<IHttpConnection>(&mockHttp.get(), [](...) {});
            CSWebSyncServerConnection connection(mockHttpConnectionPtr, hostUrl, username, password);

            // Happy path
            When(Method(mockHttp, Request)).
                Do([this,fileContent, expectedMd5, expectedSize](const HttpRequest& request) {

                HttpResponse response(200);
                Assert::AreEqual(hostUrl + L"files/remotefile/content", request.url);
                CString contentMd5Header = request.headers.value(L"Content-MD5");
                Assert::AreEqual(expectedMd5.c_str(), contentMd5Header);

                Assert::AreEqual(expectedSize, request.upload_data_size_bytes);

                std::string postedFileContent(std::istreambuf_iterator<char>(*request.upload_data), {});
                Assert::AreEqual(fileContent, postedFileContent);

                return response;
            });

            connection.putFile(srcFilePath, L"/remotefile");
            Verify(Method(mockHttp, Request)).Exactly(1);

            // http error
            When(Method(mockHttp, Request)).
                Do([fileContent, expectedMd5](const HttpRequest&) {
                return HttpResponse(500);
            });

            try {
                connection.putFile(srcFilePath, L"/remotefile");
                Assert::Fail(L"Expected sync error");
            }
            catch (const SyncError&) {
            }
        }

        TEST_METHOD(TestDownloadApplication)
        {
            std::string fileContent = "alkjasjdioajsofijasodifjwioaehfushfijsadiofjasoidjfoiasjdfoisajdfoisajf";
            wchar_t pszTempPath[MAX_PATH];
            GetTempPath(MAX_PATH, pszTempPath);
            CString destFilePath;
            destFilePath.Format(L"%szsynco-get-app-test.txt", pszTempPath);
            DeleteFile(destFilePath);

            Mock<IHttpConnection> mockHttp;

            // Happy path
            When(Method(mockHttp, Request)).
                Do([this, fileContent](const HttpRequest& request) {
                Assert::AreEqual(hostUrl + L"apps/testApp", request.url);
                HttpResponse response(200);
                response.body.observable = rxcpp::observable<>::just(fileContent);
                CString lengthHeader;
                lengthHeader.Format(L"Content-Length: %d", fileContent.length());
                response.headers.push_back(lengthHeader);
                return response;
            });

            auto mockHttpConnectionPtr = std::shared_ptr<IHttpConnection>(&mockHttp.get(), [](...) {});
            CSWebSyncServerConnection connection(mockHttpConnectionPtr, hostUrl, username, password);

            connection.downloadApplicationPackage(L"testApp", destFilePath, {}, CString());
            Verify(Method(mockHttp, Request)).Exactly(1);

            // http error
            When(Method(mockHttp, Request)).
                Do([](const HttpRequest&) {
                return HttpResponse(500);
            });

            try {
                connection.downloadApplicationPackage(L"testApp", destFilePath, {}, CString());
                Assert::Fail(L"Expected sync error");
            }
            catch (const SyncError&) {
            }
        }
    };
}
