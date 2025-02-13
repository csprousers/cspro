#include "stdafx.h"
#include "CSWebSyncServerConnection.h"
#include "CaseJsonWriter.h"
#include "CaseJsonStreamParser.h"
#include "CaseObservable.h"
#include "ConnectResponse.h"
#include "ExponentialBackoff.h"
#include "FileBasedParadataSyncer.h"
#include "ILoginDialog.h"
#include "JsonConverter.h"
#include "OauthTokenRequest.h"
#include "OauthTokenResponse.h"
#include "SyncCustomHeaders.h"
#include "SyncErrorResponse.h"
#include "SyncException.h"
#include "SyncLogCaseConstructionReporter.h"
#include "SyncRequest.h"
#include "SyncUtil.h"
#include <zNetwork/IHttpConnection.h>
#include <zToolsO/ApiKeys.h>
#include <zToolsO/base64.h>
#include <zUtilO/CredentialStore.h>
#include <zUtilO/Interapp.h>
#include <zUtilO/TemporaryFile.h>
#include <zUtilO/Versioning.h>
#include <zDictO/DDClass.h>
#include <zDataO/ISyncableDataRepository.h>
#include <zZipo/IZip.h>
#include <zZipo/ZipUtility.h>
#include <easyloggingwrapper.h>
#include <rxcpp/rx-operators.hpp>
#include <sstream>
#include <chrono>


namespace {
    JsonConverter jsonConverter;

    const float CSWEB_API_VERSION = 2.0f;

    // Valid characters for a URL - used for url encoding below
    const CString validUrlChars = _T("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.~");
    const CString validUrlCharsWithSlash = validUrlChars + _T("/");

    // Encode characters not in validChars string with % followed
    // by byte values as hex.
    CString percentEncode(LPCTSTR str, const CString& validChars)
    {
        CString encoded;
        const TCHAR* in = str;
        while (*in) {
            if (validChars.Find(*in) >= 0) {
                encoded += *in;
            } else {
                // Byte values should be based on UTF8 encoding per RFC 3986
                std::string utf8 = UTF8Convert::WideToUTF8(in, 1);
                for (std::string::const_iterator i = utf8.begin(); i != utf8.end(); ++i) {
                    encoded.AppendFormat(_T("%%%02X"), (unsigned char)*i);
                }
            }
            ++in;
        }
        return encoded;
    }

    // URL encode string
    CString urlEncode(LPCTSTR str)
    {
        // URL encoding is just percent encoding based on
        // valid url characters
        return percentEncode(str, validUrlChars);
    }

    // URL encode string but don't encode
    // the forward slashes
    CString urlEncodePath(LPCTSTR str)
    {
        return percentEncode(str, validUrlCharsWithSlash);
    }

    const std::string userAgentHeader = std::string("User-Agent: CSPro sync client/") + UTF8Convert::WideToUTF8(Versioning::GetVersionDetailedString());

    CString createAuthHeader(CString token)
    {
        return L"Authorization: Bearer " + token;
    }

    bool isRetryableHttpError(int status)
    {
        // Retry all server errors (500 and above) and
        // too many requests (429) in case of throttling
        return status >= 500 || status == 429;
    }

    // Maximum number of times to retry failed requests
    const int MAX_RETRIES = 3;

    // Number of cases to download in a single HTTP request
    const int DEFAULT_CHUNK_SIZE_GET = 100;
    const int MIN_CHUNK_SIZE_GET = 10;

    CString getHeaderFallbackToEtag(const HeaderList& responseHeaders, CString header)
    {
        CString rev = responseHeaders.value(header);
        if (rev.IsEmpty())
            rev = responseHeaders.value(_T("etag"));
        return rev;
    }

    // Extract chunk max revision from response headers
    CString getChunkMaxRevision(const HeaderList& responseHeaders)
    {
        // For newer CSWeb we use CHUNK_MAX_REVISION_HEADER but for older versions we used etag
        return getHeaderFallbackToEtag(responseHeaders, SyncCustomHeaders::CHUNK_MAX_REVISION_HEADER);
    }

    // Extract server revision from response headers
    CString getServerRevision(const HeaderList& responseHeaders)
    {
        // For newer CSWeb we use CURRENT_REVISION_HEADER but for older versions we used etag
        return getHeaderFallbackToEtag(responseHeaders, SyncCustomHeaders::CURRENT_REVISION_HEADER);
    }

    class ChunkOptimizer {
    public:
        ChunkOptimizer(CSWebDataChunk& chunk):
            m_chunk(chunk),
            m_start_time(std::chrono::high_resolution_clock::now()),
            m_num_cases(0),
            m_size_bytes(0)
        {
        }

        void OnData(size_t size_bytes, int num_cases)
        {
            m_size_bytes += size_bytes;
            m_num_cases += num_cases;
        }

        void optimizeChunk()
        {
            std::chrono::duration<float> request_duration = std::chrono::high_resolution_clock::now() - m_start_time;
            m_chunk.optimize(request_duration.count(), m_num_cases, m_size_bytes);
        }

    private:
        CSWebDataChunk& m_chunk;
        std::chrono::time_point<std::chrono::steady_clock> m_start_time;
        int m_num_cases;
        size_t m_size_bytes;
    };

    std::optional<std::pair<int, int>> parseRangeCount(const CString& range_count)
    {
        auto slash = range_count.Find('/');
        if (slash < 0)
            return {};
        int count = _ttoi((LPCTSTR)range_count.Left(slash));
        int total = _ttoi((LPCTSTR)range_count.Mid(slash + 1, range_count.GetLength() - slash));
        return std::pair(count, total);
    }
}

CSWebSyncServerConnection::CSWebSyncServerConnection(std::shared_ptr<IHttpConnection> pConnection, CString hostUrl, CString username, CString password) :
    m_pConnection(pConnection),
    m_pLoginDialog(nullptr),
    m_pSyncCredentialStore(nullptr),
    m_hostUrl(forceEndWithSlash(hostUrl)),
    m_username(username),
    m_password(password)
{

}

CSWebSyncServerConnection::CSWebSyncServerConnection(std::shared_ptr<IHttpConnection> pConnection, CString hostUrl, ILoginDialog* pLoginDlg, ICredentialStore* pCredentialStore) :
    m_pConnection(pConnection),
    m_pLoginDialog(pLoginDlg),
    m_pSyncCredentialStore(pCredentialStore),
    m_hostUrl(forceEndWithSlash(hostUrl))
{
}

CSWebSyncServerConnection::~CSWebSyncServerConnection()
{
}

ConnectResponse* CSWebSyncServerConnection::connect()
{
    CString authToken;

    if (m_pLoginDialog) {
        // No username/password specified

        // try saved auth token first
        authToken = WS2CS(m_pSyncCredentialStore->Retrieve(CS2WS(m_hostUrl)));
        m_refreshToken = WS2CS(m_pSyncCredentialStore->Retrieve(_T("refresh_") + CS2WS(m_hostUrl)));
        if (!authToken.IsEmpty()) {
            try {
                m_authHeader = createAuthHeader(authToken);
                return retrieveServerInfo();
            }
            catch (const SyncError& ex) {
                // Ignore 401 (unauthorized) errors, pass all others on
                if (ex.m_errorCode != 100101)
                    throw ex;
            }
        }

        // No saved auth token or saved token is bad

        // Keep prompting for credentials until we get
        // successfull login, cancel or error

        bool bHadFailure = false;

        while (true) {


            auto login_result = m_pLoginDialog->Show(m_hostUrl, bHadFailure);
            if (!login_result)
                throw SyncCancelException();

            const CString& username = std::get<0>(*login_result);
            const CString& password = std::get<1>(*login_result);

            try {
                retrieveOauthToken(username, password, authToken, m_refreshToken);
                m_pSyncCredentialStore->Store(CS2WS(m_hostUrl), CS2WS(authToken));
                m_pSyncCredentialStore->Store(_T("refresh_") + CS2WS(m_hostUrl), CS2WS(m_refreshToken));
                m_username = username;
                break;
            }
            catch (const SyncLoginDeniedError&) {
                // Ignore 401 (unauthorized) errors, pass all others on
                bHadFailure = true;
            }
        }

    } else {
        // Use the username/pwd passed in
        CString refreshToken;
        retrieveOauthToken(m_username, m_password, authToken, refreshToken);
    }

    m_authHeader = createAuthHeader(authToken);
    return retrieveServerInfo();
}


void CSWebSyncServerConnection::retrieveOauthToken(CString username, CString password, CString& authToken, CString& refreshToken)
{
    CString url = m_hostUrl + "token";

    HeaderList headers;
    headers.push_back(L"Content-Type: application/json; charset=utf-8");
    headers.push_back(L"Accept: application/json");
    headers.push_back(userAgentHeader.c_str());

    OauthTokenRequest request = OauthTokenRequest::CreatePasswordRequest(CSWebKeys::client_id, CSWebKeys::client_secret, username, password);
    JsonConverter converter;
    std::string requestJson = converter.toJson(request);
    std::istringstream postDataStream(requestJson);

    HttpResponse response = m_pConnection->Request(HttpRequestBuilder(url).headers(headers).post(postDataStream, requestJson.size()).build());
    std::string responseBody = response.body.ToString();

    if (response.http_status == 200) {
        try {
            std::unique_ptr<OauthTokenResponse> token_response(converter.oauthTokenResponseFromJson(responseBody));
            authToken = token_response->getAccessToken();
            refreshToken = token_response->getRefreshToken();
        }
        catch (const InvalidJsonException& e) {
            CLOG(ERROR, "sync") << "Invalid server response: " << e.what();
            CLOG(ERROR, "sync") <<  responseBody;
            throw SyncError(100121);
        }
    }
    else if (response.http_status == 401) {
        throw SyncLoginDeniedError(100126);
    }
    else {
        handleServerErrorResponse(100101, response.http_status, responseBody);
    }
}

bool CSWebSyncServerConnection::refreshOauthToken()
{
    if (m_refreshToken.IsEmpty()) {
        return false;
    }

    CLOG(INFO, "sync") << "Refreshing oauth token ";

    CString url = m_hostUrl + "token";

    HeaderList headers;
    headers.push_back(L"Content-Type: application/json; charset=utf-8");
    headers.push_back(L"Accept: application/json");
    headers.push_back(userAgentHeader.c_str());

    OauthTokenRequest request = OauthTokenRequest::CreateRefreshRequest(CSWebKeys::client_id, CSWebKeys::client_secret, m_refreshToken);
    JsonConverter converter;
    std::string requestJson = converter.toJson(request);
    std::istringstream postDataStream(requestJson);

    HttpResponse response = m_pConnection->Request(HttpRequestBuilder(url).headers(headers).post(postDataStream, requestJson.size()).build());
    std::string responseBody = response.body.ToString();

    if (response.http_status != 200) {
        CLOG(ERROR, "sync") << "Failed to refresh oauth token ";
        CLOG(ERROR, "sync") << responseBody;
        return false;
    }
    try {
        std::unique_ptr<OauthTokenResponse> token_response(converter.oauthTokenResponseFromJson(responseBody));
        CString authToken = token_response->getAccessToken();
        m_authHeader = createAuthHeader(authToken);
        m_refreshToken = token_response->getRefreshToken();
        m_pSyncCredentialStore->Store(CS2WS(m_hostUrl), CS2WS(authToken));
        m_pSyncCredentialStore->Store(_T("refresh_") + CS2WS(m_hostUrl), CS2WS(m_refreshToken));
        CLOG(INFO, "sync") << "Got new oauth token ";
        return true;
    }
    catch (const InvalidJsonException& e) {
        CLOG(ERROR, "sync") << "Invalid server response: " << e.what();
        CLOG(ERROR, "sync") << responseBody;
        return false;
    }
}

ConnectResponse* CSWebSyncServerConnection::retrieveServerInfo()
{
    CString url = m_hostUrl + "server";
    std::string responseBody;
    int status = sendRetrieveServerInfoRequest(url, responseBody);
    if (status == 401) {
        if (refreshOauthToken()) {
            status = sendRetrieveServerInfoRequest(url, responseBody);
        }
    }

    if (status == 200) {
        try {
            ConnectResponse* pResponse = jsonConverter.connectResponseFromJson(responseBody);
            CLOG(INFO, "sync") << "Server API version: " << pResponse->getApiVersion();

            m_server_api_version = pResponse->getApiVersion();

            // Check that major api versions match
            if ((int) m_server_api_version > (int) CSWEB_API_VERSION) {
                CLOG(ERROR, "sync") << "Server version " << pResponse->getApiVersion() << " does not match client version " << CSWEB_API_VERSION;
                throw SyncError(100131);
            }

            pResponse->setServerName(m_hostUrl);
            pResponse->setUserName(m_username);

            return pResponse;
        }
        catch (const InvalidJsonException& e) {
            CLOG(ERROR, "sync") << "Invalid server response: " << e.what();
            CLOG(ERROR, "sync") << responseBody;
            throw SyncError(100121);
        }
    } else {
        handleServerErrorResponse(100101, status, responseBody);
        return nullptr;
    }
}

int CSWebSyncServerConnection::sendRetrieveServerInfoRequest(CString url, std::string& responseBody)
{
    HeaderList headers;
    headers.push_back(m_authHeader);
    headers.push_back(L"Content-Type: application/json; charset=utf-8");
    headers.push_back(L"Accept: application/json");
    headers.push_back(userAgentHeader.c_str());

    HttpResponse response = m_pConnection->Request(HttpRequestBuilder(url).headers(headers).build());
    responseBody = response.body.ToString();

    return response.http_status;
}

void CSWebSyncServerConnection::disconnect()
{
}

SyncGetResponse CSWebSyncServerConnection::getData(const SyncRequest& request)
{
    const CString dictionaryName = request.getDictionary().GetName();
    CString url = m_hostUrl + "dictionaries/" + dictionaryName + "/cases";

    return downloadServerCases(url, request);
}

SyncPutResponse CSWebSyncServerConnection::putData(const SyncRequest& request)
{
    const CString dictionaryName = request.getDictionary().GetName();
    CString url = m_hostUrl + "dictionaries/" + dictionaryName + "/cases";

    std::ostringstream cases_json_stream;
    CaseJsonWriter json_writer;
    auto binaryCaseItems = request.getBinaryCaseItems();

    std::string cases_json;
    if (m_server_api_version < 2)
        json_writer.SetCSPro74BackwardsCompatible(true);
    json_writer.SetStringifyData(true);
    json_writer.ToJson(cases_json_stream, request.getClientCases());
    cases_json = cases_json_stream.str();

    if (binaryCaseItems.size() >  0) { //write the new binary json format only if cases have binary items
        std::ostringstream binary_cases_json_stream;
        JsonConverter converter;
        //cases have binary content send the questionnaire data using binary format
        converter.writeBinaryJson(binary_cases_json_stream, cases_json, binaryCaseItems);
        cases_json = binary_cases_json_stream.str();
    }

    if (m_server_api_version >= 2)
        ZipUtility::compression(cases_json);

    return uploadClientCases(url, request.getDeviceId(), request.getUniverse(),
        request.getLastServerRevision(), cases_json, request.getClientCases().size());
}

CSWebDataChunk& CSWebSyncServerConnection::getChunk()
{
    return m_dataChunk;
}

std::vector<DictionaryInfo> CSWebSyncServerConnection::getDictionaries()
{
    CString url = m_hostUrl + "dictionaries/";

    HeaderList headers;
    headers.push_back(L"Accept: application/json");
    headers.push_back(userAgentHeader.c_str());
    headers.push_back(m_authHeader);

    HttpResponse response = m_pConnection->Request(HttpRequestBuilder(url).headers(headers).build());
    std::string responseBody = response.body.ToString();

    if (response.http_status == 200) {
        try {
            return jsonConverter.dictionaryListFromJson(responseBody);
        }
        catch (const InvalidJsonException& e) {
            CLOG(ERROR, "sync") << "Invalid server response: " << e.what();
            CLOG(ERROR, "sync") << responseBody;
            throw SyncError(100121);
        }
    } else {
        handleServerErrorResponse(100129, response.http_status, responseBody);
    }
    return std::vector<DictionaryInfo>();
}

CString CSWebSyncServerConnection::getDictionary(CString dictionaryName)
{
    CString url = m_hostUrl + "dictionaries/" + dictionaryName;

    HeaderList headers;
    headers.push_back(L"Accept: application/json");
    headers.push_back(userAgentHeader.c_str());
    headers.push_back(m_authHeader);

    HttpResponse response = m_pConnection->Request(HttpRequestBuilder(url).headers(headers).build());
    std::string response_body = response.body.ToString();

    if (response.http_status == 200) {
        return UTF8Convert::UTF8ToWide<CString>(response_body);
    } else {
        handleServerErrorResponse(100130, response.http_status, response_body);
    }
    return CString();
}

void CSWebSyncServerConnection::putDictionary(CString dictPath)
{
    CString url = m_hostUrl + "dictionaries/";

    HeaderList headers;
    headers.push_back(L"Content-Type: text/plain; charset=utf-8");
    headers.push_back(L"Accept: application/json");
    headers.push_back(userAgentHeader.c_str());
    headers.push_back(m_authHeader);

    std::ifstream uploadStream(dictPath, std::ios::binary);
    int64_t fileSizeBytes = PortableFunctions::FileSize(dictPath);

    auto response = m_pConnection->Request(HttpRequestBuilder(url).headers(headers).post(uploadStream, fileSizeBytes).build());
    std::string responseBody = response.body.ToString();

    if (response.http_status != 200) {
        handleServerErrorResponse(100143, response.http_status, responseBody);
    }
}

void CSWebSyncServerConnection::deleteDictionary(CString dictionaryName)
{
    CString url = m_hostUrl + "dictionaries/" + dictionaryName;

    HeaderList headers;
    headers.push_back(L"Accept: application/json");
    headers.push_back(userAgentHeader.c_str());
    headers.push_back(m_authHeader);

    std::ostringstream result;
    HeaderList responseHeaders;

    auto response = m_pConnection->Request(HttpRequestBuilder(url).headers(headers).del().build());

    if (response.http_status != 200) {
        handleServerErrorResponse(100101, response.http_status, response.body.ToString());
    }
}

SyncGetResponse CSWebSyncServerConnection::downloadServerCases(CString url, const SyncRequest& request)
{
    CString lastGuid = request.getLastCaseUuid();

    CLOG(INFO, "sync") << "Start case download with chunk size of " << m_dataChunk.getSize();

    CString serverRevision = request.getLastServerRevision();

    if (!lastGuid.IsEmpty())
        CLOG(INFO, "sync") << "Download chunk starting from case " << UTF8Convert::WideToUTF8(lastGuid);

    auto json_parser = std::make_shared<CaseJsonStreamParser>(request.getCaseAccess(), std::make_shared<SyncLogCaseConstructionReporter>());

    while (true) {

        std::shared_ptr<ChunkOptimizer> chunk_optimizer = std::make_shared<ChunkOptimizer>(m_dataChunk);

        auto http_response = sendDownloadServerCasesRequest(url, request.getDeviceId(),
                        request.getUniverse(), serverRevision, lastGuid,
                        request.getExcludedServerRevisions());

        CLOG(INFO, "sync") << "Status : " << http_response.http_status;
        if (http_response.http_status == 200 || http_response.http_status == 206) {
            CLOG(INFO, "sync") << "Server case count = " << UTF8Convert::WideToUTF8(http_response.headers.value(SyncCustomHeaders::RANGE_COUNT_HEADER));
            SyncGetResponse::SyncGetResult result = http_response.http_status == 200 ? SyncGetResponse::SyncGetResult::Complete : SyncGetResponse::SyncGetResult::MoreData;
            serverRevision = getChunkMaxRevision(http_response.headers);
            if (serverRevision.IsEmpty()) {
                CLOG(ERROR, "sync") << "Server response missing revision header " << SyncCustomHeaders::CHUNK_MAX_REVISION_HEADER;
                CLOG(ERROR, "sync") << "HEADERS:";
                for (auto header : http_response.headers) {
                    CLOG(ERROR, "sync") << (LPCTSTR)header;
                }
                CLOG(ERROR, "sync") << "BODY:";
                CLOG(ERROR, "sync") << http_response.body.ToString();
                throw SyncError(100121);
            } 
            
            auto observable = std::make_shared<CaseObservable>(rxcpp::observable<>::create<std::shared_ptr<Case>>([json_parser,http_response,chunk_optimizer](rxcpp::subscriber<std::shared_ptr<Case>> case_subscriber) {
                bool isBinaryJSON = false;
                int  totalBytes = 0;
                std::stringstream iss;
                http_response.body.observable.subscribe(
                            [json_parser, &case_subscriber, chunk_optimizer, &isBinaryJSON, &iss, &totalBytes](std::string s) {
                                JsonConverter converter;
                                if (isBinaryJSON) {
                                    //For binary json format we need to save the entire stream response before processing the cases
                                    //and binary content
                                    iss.seekg(0, iss.end);
                                    iss.write(reinterpret_cast<const char*>(s.data()), s.size());
                                    totalBytes += s.size();
                                    return;
                                }
                                size_t chunkSize = s.size();
                                if (!isBinaryJSON && converter.IsBinaryJson(s)) {
                                    //read binary cases json
                                    isBinaryJSON = true;
                                    totalBytes += s.size();
                                    iss.write(reinterpret_cast<const char*>(s.data()), s.size());
                                    return;
                                }
                               
                                std::vector<std::shared_ptr<Case>> cases = json_parser->Parse(s);
                                for (const std::shared_ptr<Case>& data_case : cases) {
                                    case_subscriber.on_next(data_case);
                                }
                                chunk_optimizer->OnData(chunkSize, cases.size());
                            },
                            [](std::exception_ptr eptr) {
                                try {
                                    std::rethrow_exception(eptr);
                                }
                                catch (const SyncError & e) {
                                    CLOG(ERROR, "sync") << "SyncError reading server cases: " << e.what();
                                    throw;
                                }
                                catch (const jsoncons::json_exception& e) {
                                    CLOG(ERROR, "sync") << "Invalid JSON parsing server cases: " << e.what();
                                    throw SyncError(100121);
                                }
                                catch (const std::exception & e) {
                                    CLOG(ERROR, "sync") << "Error reading server cases: " << e.what();
                                    throw SyncError(100121);
                                }
                            },
                            [json_parser, &case_subscriber, &http_response, chunk_optimizer, &isBinaryJSON, &iss]() {
                                if (isBinaryJSON) {
                                    //parse the json cases
                                    //for each case get the binary content and then call the case subscribers next
                                    JsonConverter converter;
                                    int totalCases = 0;
                                    std::istringstream binarySS(iss.str());

                                    //get the total bytes
                                    binarySS.seekg(0, binarySS.end);
                                    int totalByteSize = binarySS.tellg();
                                    binarySS.seekg(0, binarySS.beg);

                                    //get the questionnaire string
                                    std::string jsonCases = converter.readCasesJsonFromBinary(binarySS);
                                    std::string_view str_view(jsonCases);
                                    std::size_t chunk_size = 16*1024;

                                    for (std::size_t i = 0; i < str_view.size(); i += chunk_size) {
                                        std::string_view jsonCasesChunk = str_view.substr(i, chunk_size);
                                        // Process the chunk here
                                        std::vector<std::shared_ptr<Case>> cases = json_parser->Parse(jsonCasesChunk);
                                        //if this is the last chunk call end parse
                                        if (str_view.size() < i + chunk_size) {
                                            //this being called only for any subscribers of the parser to be notified. There should be no
                                            //more cases returned in here as the final chunk is parsed in the above call
                                           auto parsed_cases = json_parser->EndParse();
                                           ASSERT(parsed_cases.empty());
                                        }
                                        for (const std::shared_ptr<Case>& data_case : cases) {
                                            //load the binary items for the case before notifying the caseobservable subscriber
                                            if (isBinaryJSON) {
                                                converter.readBinaryItemsForCase(binarySS, *data_case);
                                            }
                                            case_subscriber.on_next(data_case);
                                            totalCases++;
                                        }
                                    }
                                    chunk_optimizer->OnData(totalByteSize, totalCases);
                                }
                                else {
                                    std::vector<std::shared_ptr<Case>> cases = json_parser->EndParse();
                                    for (const std::shared_ptr<Case>& data_case : cases) {
                                        case_subscriber.on_next(data_case);
                                    }
                                    chunk_optimizer->OnData(0, cases.size());
                                }
                                if (http_response.http_status == 206) // Only increase chunk size if chunk is full
                                    chunk_optimizer->optimizeChunk();
                                case_subscriber.on_completed();
                            });
                }));

            auto range_count = parseRangeCount(http_response.headers.value(SyncCustomHeaders::RANGE_COUNT_HEADER));
            if (range_count)
                return SyncGetResponse(result, observable, serverRevision, range_count->second);
            else
                return SyncGetResponse(result, observable, serverRevision);

        } else if (http_response.http_status == 401) {
            // Get new oauth token and try again
            if (!refreshOauthToken()) {
                handleServerErrorResponse(100101, http_response.http_status, http_response.body.ToString());
            }
        } else if (http_response.http_status == 412) {
            return SyncGetResponse::SyncGetResult::RevisionNotFound;
        } else {
            handleServerErrorResponse(100101, http_response.http_status, http_response.body.ToString());
        }
    }
}

SyncPutResponse CSWebSyncServerConnection::uploadClientCases(CString url, CString deviceId,
    CString universe, CString lastServerRevision,
    const std::string& clientCasesJson, int numberOfCases)
{
    std::string responseBody;
    HeaderList responseHeaders;
    float requestTimeSecs;

    int status = sendUploadClientCasesRequest(url, clientCasesJson, deviceId, universe, lastServerRevision, responseHeaders, responseBody, requestTimeSecs);
    if (status == 401) {
        if (refreshOauthToken()) {
            status = sendUploadClientCasesRequest(url, clientCasesJson, deviceId, universe, lastServerRevision, responseHeaders, responseBody, requestTimeSecs);
        }
    }

    if (status == 200) {
        CString serverRevision = getServerRevision(responseHeaders);
        if (serverRevision.IsEmpty()) {
            CLOG(ERROR, "sync") << "Server response missing revision header " << SyncCustomHeaders::CURRENT_REVISION_HEADER;
            CLOG(ERROR, "sync") << "HEADERS:";
            for (auto header : responseHeaders) {
                CLOG(ERROR, "sync") << (LPCTSTR)header;
            }
            CLOG(ERROR, "sync") << "BODY:";
            CLOG(ERROR, "sync") << responseBody;
            throw SyncError(100121);
        }

        m_dataChunk.optimize(requestTimeSecs, numberOfCases, clientCasesJson.size());
        return SyncPutResponse(SyncPutResponse::SyncPutResult::Complete, getServerRevision(responseHeaders));
    } else if (status == 412) {
        return SyncPutResponse(SyncPutResponse::SyncPutResult::RevisionNotFound);
    } else {
        handleServerErrorResponse(100101, status, responseBody);
        // handleServerErrorResponse throws so we never get here but need this to avoid compiler warning
        return SyncPutResponse(SyncPutResponse::SyncPutResult::Complete);
    }
}

int CSWebSyncServerConnection::sendUploadClientCasesRequest(CString url, const std::string& requestBody, CString deviceId, CString universe, CString lastServerRevision, HeaderList& responseHeaders, std::string& responseBody, float& requestTimeSecs)
{
    HeaderList headers;
    headers.push_back(m_authHeader);
    headers.push_back(L"Content-Type: application/json; charset=utf-8");
    if (m_server_api_version >= 2)
        headers.push_back(L"Content-Encoding: gzip");
    headers.push_back(L"Accept: application/json");
    headers.push_back(L"Cookie: XDEBUG_SESSION=netbeans-xdebug");
    headers.push_back(userAgentHeader.c_str());
    headers.push_back(SyncCustomHeaders::DEVICE_ID_HEADER + _T(": ") + deviceId);
    if (!lastServerRevision.IsEmpty())
        headers.push_back(SyncCustomHeaders::IF_REVISION_EXISTS_HEADER + _T(':') + lastServerRevision);

    ExponentialBackOff backOff;

    int retries = 0;
    while (true) {

        try {
            std::istringstream postDataStream(requestBody);
            auto requestStartTime = std::chrono::high_resolution_clock::now();

            auto response = m_pConnection->Request(HttpRequestBuilder(url).headers(headers).post(postDataStream, requestBody.size()).build());

            responseBody = response.body.ToString();

            std::chrono::duration<float> requestDuration = std::chrono::high_resolution_clock::now() - requestStartTime;
            requestTimeSecs = requestDuration.count();

            responseHeaders = response.headers;

            if (!isRetryableHttpError(response.http_status)) {
                return response.http_status;
            }
            CLOG(ERROR, "sync") << "HTTP error: " << response.http_status;
            CLOG(ERROR, "sync") << responseBody;
            if (retries == MAX_RETRIES)
                break;
        }
        catch (const SyncRetryableNetworkError& ex) {
            CLOG(ERROR, "sync") << "Network error: " << ex.what();
            if (retries == MAX_RETRIES)
                throw ex;
        }

        m_dataChunk.onError();
        ++retries;
        CLOG(INFO, "sync") << "Retrying. Attempt #" << retries;
        Sleep(backOff.NextBackOffMillis());

    }

    return 500; // should never get here
}

HttpResponse CSWebSyncServerConnection::sendDownloadServerCasesRequest(CString url, CString deviceId, CString universe,
    CString lastServerRevision, CString lastGuid,
    const std::vector<CString>& excludeRevisions)
{
    HeaderList headers;
    headers.push_back(m_authHeader);
    headers.push_back(L"Content-Type: application/json; charset=utf-8");
    headers.push_back(L"Accept: application/json");
    headers.push_back(L"Cookie: XDEBUG_SESSION=netbeans-xdebug");
    headers.push_back(userAgentHeader.c_str());
    headers.push_back(L"Cache-Control: no-cache, no-store, must-revalidate");
    headers.push_back(SyncCustomHeaders::DEVICE_ID_HEADER + _T(": ") + deviceId);
    if (!universe.IsEmpty())
        headers.push_back(SyncCustomHeaders::UNIVERSE_HEADER + _T(": \"") + universe + _T('\"'));
    if (!lastGuid.IsEmpty())
        headers.push_back(SyncCustomHeaders::START_AFTER_HEADER + _T(": ") + lastGuid);
    if (!excludeRevisions.empty()) {
        CString excludeHeader = SyncCustomHeaders::EXCLUDE_REVISIONS_HEADER + _T(": ") + WS2CS(SO::CreateSingleString(excludeRevisions, _T(",")));
        headers.push_back(excludeHeader);
    }
    CString countHeader;
    countHeader.Format(_T("%s: %d"), (LPCTSTR)SyncCustomHeaders::RANGE_COUNT_HEADER, m_dataChunk.getSize());
    headers.push_back(countHeader);

    if (!lastServerRevision.IsEmpty())
        headers.push_back(SyncCustomHeaders::IF_REVISION_EXISTS_HEADER + _T(':') + lastServerRevision);
    std::ostringstream result;

    ExponentialBackOff backOff;

    int retries = 0;
    while (true) {

        try {

            auto response = m_pConnection->Request(HttpRequestBuilder(url).headers(headers).build());

            if (!isRetryableHttpError(response.http_status)) {
                return response;
            }
            CLOG(ERROR, "sync") << "HTTP error: " << response.http_status;
            CLOG(ERROR, "sync") << response.body.ToString();
            if (retries == MAX_RETRIES)
                break;
        }
        catch (const SyncRetryableNetworkError& ex) {
            CLOG(ERROR, "sync") << "Network error: " << ex.what();
            if (retries == MAX_RETRIES)
                throw ex;
        }

        m_dataChunk.onError();
        ++retries;
        CLOG(INFO, "sync") << "Retrying. Attempt #" << retries;
        Sleep(backOff.NextBackOffMillis());

    }

    return 500; // should never get here
}

bool CSWebSyncServerConnection::getFile(CString remoteFilePath, CString localFilePath, CString /*actualLocalFilePath*/, CString md5 /*= CString()*/)
{
    CString url = m_hostUrl;
    url = PortableFunctions::PathAppendForwardSlashToPath(url, L"files");
    url = PortableFunctions::PathAppendForwardSlashToPath(url, urlEncodePath(remoteFilePath));
    url = PortableFunctions::PathAppendForwardSlashToPath(url, L"content");

    HeaderList responseHeaders;
    HeaderList requestHeaders;

    int status = sendGetFileRequest(url, localFilePath, md5, requestHeaders, responseHeaders);
    if (status == 401) {
        if (refreshOauthToken()) {
            responseHeaders = HeaderList();
            status = sendGetFileRequest(url, localFilePath, md5, requestHeaders, responseHeaders);
        }
    }

    if (status != 200 && status != 304) {
        throw SyncError(100110, remoteFilePath);
    }

    if (status == 200) {

        // Check that downloaded file matches signature in response headers
        CString serverMd5 = responseHeaders.value(L"Content-MD5");
        if (!serverMd5.IsEmpty()) {
            std::wstring clientMd5 = PortableFunctions::FileMd5(localFilePath);
            if (!SO::EqualsNoCase(clientMd5, serverMd5)) {
                throw SyncError(100127, remoteFilePath);
            }
        }
    }

    return status == 200;
}

std::unique_ptr<TemporaryFile> CSWebSyncServerConnection::getFileIfExists(const CString& remoteFilePath)
{
    try
    {
        auto temporary_file = std::make_unique<TemporaryFile>();

        if( getFile(remoteFilePath, WS2CS(temporary_file->GetPath()), WS2CS(temporary_file->GetPath()), CString()) )
            return temporary_file;
    }

    catch( const SyncError& sync_error )
    {
        // rethrow the error if it was anything other than "Error downloading file"
        if( sync_error.m_errorCode != 100110 )
            throw;
    }

    return nullptr;
}

int CSWebSyncServerConnection::sendGetFileRequest(CString url, CString localFilePath, CString etag, const HeaderList& additionalRequestHeaders, HeaderList& responseHeaders)
{
    HeaderList headers;
    headers.push_back(m_authHeader);
    if (!etag.IsEmpty())
        headers.push_back(CString(_T("If-None-Match: ")) + etag);
    headers.push_back(L"Accept: application/octet-stream");
    headers.push_back(userAgentHeader.c_str());
    headers.append(additionalRequestHeaders);

    auto response = m_pConnection->Request(HttpRequestBuilder(url).headers(headers).build());
    responseHeaders = response.headers;
    if (response.http_status == 200) {
        std::ofstream fileStream(localFilePath, std::ios::binary);
        fileStream << response.body;
    }
    else if (response.http_status != 304) {
        CLOG(ERROR, "sync") << "Failed to download file to " << localFilePath;
        CLOG(ERROR, "sync") << response.body.ToString();
    }
    return response.http_status;
}

void CSWebSyncServerConnection::putFile(CString localPath, CString remotePath)
{
    int64_t fileSizeBytes = PortableFunctions::FileSize(localPath);
    CLOG(INFO, "sync") << "Uploading " << fileSizeBytes << " bytes";

#ifdef ANDROID
    // Due to limitations in Android httpconnection API we can only post
    // data up to 2GB. The Java method HttpUrlConnection.setFixedLengthStreamingMode
    // takes a Java int. In Android API 19 a method that takes a long was added but
    // in order to support earlier versions of Android we use the older version
    // of the method that takes an int which means we can only handle posts up to 2GB.
    if (fileSizeBytes > 0x7FFFFFFF) {
        throw SyncError(100117, localPath);
    }
#endif

    CString url = m_hostUrl;
    url = PortableFunctions::PathAppendForwardSlashToPath(url, L"files");
    url = PortableFunctions::PathAppendForwardSlashToPath(url, urlEncodePath(remotePath));
    url = PortableFunctions::PathAppendForwardSlashToPath(url, L"content");

    std::wstring md5 = PortableFunctions::FileMd5(localPath);

    std::string responseBody;
    int status = sendPutFileRequest(url, localPath, fileSizeBytes, WS2CS(md5), responseBody);
    if (status == 401) {
        if (refreshOauthToken()) {
            status = sendPutFileRequest(url, localPath, fileSizeBytes, WS2CS(md5), responseBody);
        }
    }

    if (status != 200) {
        try {
            // try to parse error from http response
            JsonConverter converter;
            std::unique_ptr<SyncErrorResponse> response(converter.syncErrorResponseFromJson(responseBody));
            CString msg;
            msg.Format(L"\"%s\": %s (%d)", (LPCTSTR)remotePath, (LPCTSTR)response->GetErrorDescription(), status);
            throw SyncError(100111, msg);
        }
        catch (const InvalidJsonException& e) {
            CLOG(ERROR, "sync") << "Invalid server response: " << e.what();
            CLOG(ERROR, "sync") << responseBody;
            // If can't parse message then just use the http error code. Don't show the body since
            // it is probably html.
            CString msg;
            msg.Format(L"\"%s\": Error %d", (LPCTSTR)remotePath, status);
            throw SyncError(100111, msg);
        }
    }
}

int CSWebSyncServerConnection::sendPutFileRequest(CString url, CString localFilePath, int64_t fileSizeBytes, CString md5, std::string& responseBody)
{
    std::ifstream uploadStream(localFilePath, std::ios::binary);
    HeaderList headers;
    headers.push_back(m_authHeader);
    headers.push_back(L"Content-Type: application/octet-stream");
    if (!md5.IsEmpty())
        headers.push_back(L"Content-MD5: " + md5);
    headers.push_back(L"Accept: application/json");
    headers.push_back(userAgentHeader.c_str());

    auto response = m_pConnection->Request(HttpRequestBuilder(url).headers(headers).put(uploadStream, fileSizeBytes).build());
    responseBody = response.body.ToString();
    return response.http_status;
}

std::vector<FileInfo>* CSWebSyncServerConnection::getDirectoryListing(CString remotePath)
{
    CString url = m_hostUrl;
    url = PortableFunctions::PathAppendForwardSlashToPath(url, L"folders");
    url = PortableFunctions::PathAppendForwardSlashToPath(url, urlEncodePath(remotePath));

    std::string responseBody;
    int status = sendGetJsonRequest(url, responseBody);
    if (status == 401) {
        if (refreshOauthToken()) {
            status = sendGetJsonRequest(url, responseBody);
        }
    }

    if (status == 200) {
        try {
            return jsonConverter.fileInfoFromJson(responseBody);
        }
        catch (const InvalidJsonException& e) {
            CLOG(ERROR, "sync") << "Invalid server response: " << e.what();
            CLOG(ERROR, "sync") << responseBody;
            throw SyncError(100121);
        }
    }
    else {
        try {
            // try to parse error from http response
            JsonConverter converter;
            std::unique_ptr<SyncErrorResponse> response(converter.syncErrorResponseFromJson(responseBody));
            CString msg;
            msg.Format(L"\"%s\": %s (%d)", (LPCTSTR)remotePath, (LPCTSTR)response->GetErrorDescription(), status);
            throw SyncError(100112, msg);
        }
        catch (const InvalidJsonException& e) {
            CLOG(ERROR, "sync") << "Invalid server response: " << e.what();
            CLOG(ERROR, "sync") << responseBody;
            // If can't parse message then just use the http error code. Don't show the body since
            // it is probably html.
            CString msg;
            msg.Format(L"\"%s\": Error %d", (LPCTSTR)remotePath, status);
            throw SyncError(100112, msg);
        }
    }
}

std::vector<ApplicationPackage> CSWebSyncServerConnection::listApplicationPackages()
{
    CString url = m_hostUrl;
    url = PortableFunctions::PathAppendForwardSlashToPath(url, L"apps");

    std::string responseBody;
    int status = sendGetJsonRequest(url, responseBody);
    if (status == 401) {
        if (refreshOauthToken()) {
            status = sendGetJsonRequest(url, responseBody);
        }
    }

    if (status == 200) {
        try {
            return jsonConverter.applicationPackageListFromJson(responseBody);
        }
        catch (const InvalidJsonException& e) {
            CLOG(ERROR, "sync") << "Invalid server response: " << e.what();
            CLOG(ERROR, "sync") << responseBody;
            throw SyncError(100121);
        }
    }
    else {
        try {
            // try to parse error from http response
            JsonConverter converter;
            std::unique_ptr<SyncErrorResponse> response(converter.syncErrorResponseFromJson(responseBody));
            if (status == 404 && response->GetError() == _T("fatal_error") && response->GetErrorDescription().Left(18) == _T("No route found for"))
            {
                // Old version of server that doesn't have app list endpoint
                throw SyncError(100142);
            }
            else
            {
                CString msg;
                msg.Format(L"%s (%d)", (LPCTSTR)response->GetErrorDescription(), status);
                throw SyncError(100141, msg);
            }
        }
        catch (const InvalidJsonException& e) {
            CLOG(ERROR, "sync") << "Invalid server response: " << e.what();
            CLOG(ERROR, "sync") << responseBody;
            throw SyncError(100121);
        }
    }
}

bool CSWebSyncServerConnection::downloadApplicationPackage(CString packageName, CString localPath, const std::optional<ApplicationPackage>& currentPackage, const CString& currentPackageSignature)
{
    CString url = m_hostUrl;
    url = PortableFunctions::PathAppendForwardSlashToPath(url, L"apps");
    url = PortableFunctions::PathAppendForwardSlashToPath(url, urlEncodePath(packageName));

    HeaderList responseHeaders;
    HeaderList requestHeaders;

    if (currentPackage) {
        // For an existing package we the build time and the package file list in headers
        requestHeaders.push_back(SyncCustomHeaders::APP_PACKAGE_BUILD_TIME_HEADER + + _T(": ") + PortableFunctions::TimeToRFC3339String(currentPackage->getBuildTime()));

        // Convert the file list to base64 encoded, compressed json
        auto packageFiles = currentPackage->getFiles();
        // No need to send the only on first install flag to server, setting to false removes from JSON
        for (auto& f : packageFiles) {
            f.OnlyOnFirstInstall = false;
        }

        std::string fileJson = jsonConverter.toJson(packageFiles);
        if (ZipUtility::compression(fileJson)) {
            CLOG(ERROR, "sync") << "Failed to compress package json, fallback to full package download";
        } else {
            requestHeaders.push_back(SyncCustomHeaders::APP_PACKAGE_FILES_HEADER + _T(": ") +
                Base64::Encode<std::wstring>(fileJson.data(), fileJson.length()).c_str());
        }
    }

    TemporaryFile tmpZip(PortableFunctions::PathGetDirectory<CString>(localPath));
    int status = sendGetFileRequest(url, WS2CS(tmpZip.GetPath()), currentPackageSignature, requestHeaders, responseHeaders);
    if (status == 401) {
        if (refreshOauthToken()) {
            responseHeaders = HeaderList();
            status = sendGetFileRequest(url, WS2CS(tmpZip.GetPath()), currentPackageSignature, requestHeaders, responseHeaders);
        }
    }

    if (status != 200 && status != 304) {
        if (status == 400 && currentPackage && requestHeaders.value(SyncCustomHeaders::APP_PACKAGE_FILES_HEADER).GetLength() >= 8000) {
            // bad request - likely due to big header
            CLOG(ERROR, "sync") << "Bad request - size of request header is "
                << requestHeaders.value(SyncCustomHeaders::APP_PACKAGE_FILES_HEADER).GetLength()
                << " bytes. That could be greater than the max allowable header on the server (default of 8kb in Apache)."
                << "Try increasing LimitRequestFieldSize in the Apache config file on the server or decreasing the number of files in your package.";
            throw SyncError(100158, packageName);
        }
        throw SyncError(100157, packageName);
    }

    if (status == 304) {
        CLOG(INFO, "sync") << "Application package already up to date ";
        return false;
    } else {
        if (!tmpZip.Rename(CS2WS(localPath))) {
            throw SyncError(100109, localPath);
        }
        CLOG(INFO, "sync") << "Downloaded application package " << UTF8Convert::WideToUTF8(packageName);
        return true;
    }
}

void CSWebSyncServerConnection::uploadApplicationPackage(CString localPath, CString packageName, CString packageSpecJson)
{
    int64_t fileSizeBytes = PortableFunctions::FileSize(localPath);
    CLOG(INFO, "sync") << "Uploading app package " << UTF8Convert::WideToUTF8(packageName) << " " << fileSizeBytes << " bytes";

#ifdef ANDROID
    // Due to limitations in Android httpconnection API we can only post
    // data up to 2GB. The Java method HttpUrlConnection.setFixedLengthStreamingMode
    // takes a Java int. In Android API 19 a method that takes a long was added but
    // in order to support earlier versions of Android we use the older version
    // of the method that takes an int which means we can only handle posts up to 2GB.
    if (fileSizeBytes > 0x7FFFFFFF) {
        throw SyncError(100117, localPath);
    }
#endif

    CString url = m_hostUrl;
    url = PortableFunctions::PathAppendForwardSlashToPath(url, L"apps");
    url = PortableFunctions::PathAppendForwardSlashToPath(url, urlEncodePath(packageName));

    std::string responseBody;
    int status = sendPutFileRequest(url, localPath, fileSizeBytes, CString(), responseBody);
    if (status == 401) {
        if (refreshOauthToken()) {
            status = sendPutFileRequest(url, localPath, fileSizeBytes, CString(), responseBody);
        }
    }

    if (status != 200) {
        try {
            // try to parse error from http response
            JsonConverter converter;
            std::unique_ptr<SyncErrorResponse> response(converter.syncErrorResponseFromJson(responseBody));
            if (status == 404 && response->GetError() == _T("fatal_error") && response->GetErrorDescription().Left(18) == _T("No route found for"))
            {
                // Old version of server that doesn't have app upload endpoint
                throw SyncError(100142);
            }
            else
            {
                CString msg;
                msg.Format(L"\"%s\": %s (%d)", (LPCTSTR)packageName, (LPCTSTR)response->GetErrorDescription(), status);
                throw SyncError(100140, msg);
            }
        }
        catch (const InvalidJsonException& e) {
            CLOG(ERROR, "sync") << "Invalid server response: " << e.what();
            CLOG(ERROR, "sync") << responseBody;
            throw SyncError(100121);
        }
    }
}

CString CSWebSyncServerConnection::syncMessage(const CString&, const CString&)
{
    throw SyncError(100154);
}

void CSWebSyncServerConnection::setDownloadPffServerParams(PFF& pff)
{
    pff.SetSyncServerType(SyncServerType::CSWeb);
    pff.SetSyncUrl(m_hostUrl);
}

int CSWebSyncServerConnection::sendGetJsonRequest(CString url, std::string& responseBody)
{
    HeaderList headers;
    headers.push_back(m_authHeader);
    headers.push_back(L"Content-Type: application/json; charset=utf-8");
    headers.push_back(L"Accept: application/json");
    headers.push_back(userAgentHeader.c_str());

    auto response = m_pConnection->Request(HttpRequestBuilder(url).headers(headers).build());
    responseBody = response.body.ToString();
    return response.http_status;
}

void CSWebSyncServerConnection::setListener(ISyncListener* listener)
{
    m_pConnection->setListener(listener);
}

void CSWebSyncServerConnection::handleServerErrorResponse(int msgNum, int httpResponseCode, const std::string &responseBody)
{
    try {
        // try to parse error from http response
        JsonConverter converter;
        std::unique_ptr<SyncErrorResponse> response(converter.syncErrorResponseFromJson(responseBody));
        CString msg;
        msg.Format(L"%s. (%d)", (LPCTSTR)response->GetErrorDescription(), httpResponseCode);
        throw SyncError(msgNum, msg);
    }
    catch (const InvalidJsonException&) {
        CLOG(ERROR, "sync") << "Error from server: ";
        CLOG(ERROR, "sync") << responseBody;

        // If can't parse message then just use the http error code. Don't show the body since
        // it is probably html.
        if (httpResponseCode == 404) {
            // This is most likely an invalid server URL
            throw SyncError(100144, m_hostUrl);
        }
        else {
            CString msg;
            msg.Format(L"Error %d", httpResponseCode);
            throw SyncError(msgNum, msg);
        }
    }
}


CString CSWebSyncServerConnection::startParadataSync(const CString& log_uuid)
{
    m_paradataSyncer = std::make_unique<FileBasedParadataSyncer<CSWebSyncServerConnection>>(*this, _T("/paradata/"));

    return m_paradataSyncer->startParadataSync(log_uuid);
}

void CSWebSyncServerConnection::putParadata(const CString& filename)
{
    return m_paradataSyncer->putParadata(filename);
}

std::vector<std::shared_ptr<TemporaryFile>> CSWebSyncServerConnection::getParadata()
{
    return m_paradataSyncer->getParadata();
}

void CSWebSyncServerConnection::stopParadataSync()
{
    m_paradataSyncer->stopParadataSync();

    m_paradataSyncer.reset();
}
