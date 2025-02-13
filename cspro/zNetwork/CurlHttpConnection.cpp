#include "stdafx.h"
#include "CurlHttpConnection.h"
#include <zSyncO/ISyncListener.h>
#include <zSyncO/SyncException.h>
#include <curl/curl.h>

namespace {
    struct curl_slist* createHeaderList(const HeaderList& headers)
    {
        struct curl_slist* list = NULL;
        for (HeaderList::const_iterator i = headers.begin(); i != headers.end(); ++i) {
            list = curl_slist_append(list, UTF8Convert::WideToUTF8(*i).c_str());
        }
        return list;
    }
}

class CurlOperationState {

public:

    CurlOperationState(HttpRequest request, ISyncListener* listener) :
        m_request(request),
        m_easy_handle(curl_easy_init()),
        m_curl_request_headers(createHeaderList(request.headers)),
        m_finished_reading_headers(false),
        m_listener(listener)
    {
        m_error_message.resize(CURL_ERROR_SIZE);
    }

    ~CurlOperationState()
    {
        if (m_curl_request_headers)
            curl_slist_free_all(m_curl_request_headers);
        curl_easy_cleanup(m_easy_handle);
    }

    HttpRequest m_request;
    CURL* m_easy_handle;
    std::optional<rxcpp::subscriber<std::string>> m_body_subscriber;
    struct curl_slist* m_curl_request_headers;
    HeaderList m_response_headers;
    bool m_finished_reading_headers;
    std::string m_error_message;
    ISyncListener* m_listener;
};

namespace {
    bool IsContinueHeader(const CString& header)
    {
        return header.GetLength() > 12 && header.Left(5) == "HTTP/" && header.Right(12) == "100 Continue";
    }

    int writeCallback(char* data, size_t size, size_t nmemb, CurlOperationState* state)
    {
        if (state->m_body_subscriber) {
            auto s = std::string(data, size * nmemb);
            OutputDebugStringA(s.c_str());
            (*state->m_body_subscriber).on_next(std::string(data, size * nmemb));
            return size * nmemb;
        }
        else
        {
            state->m_finished_reading_headers = true;
            return CURL_WRITEFUNC_PAUSE;
        }
    }

    size_t readCallback(void* ptr, size_t size, size_t nmemb, CurlOperationState* state)
    {
        if (size * nmemb < 1)
            return 0;

        std::istream& is = *state->m_request.upload_data;

        if (is.eof())
            return 0;

        if (is.fail() || is.bad()) {
            return CURL_READFUNC_ABORT;
        }

        is.read((char*)ptr, size * nmemb);

        return (size_t)is.gcount();
    }

    int seekCallback(CurlOperationState* state, curl_off_t offset, int origin)
    {
        std::ios_base::seekdir whence;

        switch (origin)
        {
        case SEEK_SET:
            whence = std::ios_base::beg;
            break;
        case SEEK_CUR:
            whence = std::ios_base::cur;
            break;
        case SEEK_END:
            whence = std::ios_base::end;
            break;
        default:
            return CURL_SEEKFUNC_CANTSEEK;
        }

        std::istream& is = *state->m_request.upload_data;
        is.clear(); // clear errors from last read so we can see if seek works
        is.seekg(offset, whence);
        if (is.fail() || is.bad()) {
            is.clear(); // Clear seek error so curl can try something else (docs hint that it has other methods)
            return CURL_SEEKFUNC_CANTSEEK;
        }

        return CURL_SEEKFUNC_OK;
    }

    size_t headerCallback(char* data, size_t size, size_t nmemb, CurlOperationState* state)
    {
        CString header = UTF8Convert::UTF8ToWide<CString>(std::string_view(data, size * nmemb));
        header.TrimRight(); // remove the trailing crlf
        if (header.IsEmpty()) {
            // An empty header signals end of headers, but not if it is after a "HTTP/1.1 100 Continue" which comes before reading input data
            if (!state->m_response_headers.empty()) {
                const CString& last_header = state->m_response_headers.at(state->m_response_headers.size() - 1);
                if (!IsContinueHeader(last_header))
                    state->m_finished_reading_headers = true;
            }
        }
        else {
            state->m_response_headers.push_back(header);
        }
        return size * nmemb;
    }

    int progressCallback(CurlOperationState* state, curl_off_t dltotal, curl_off_t dlnow,
        curl_off_t ultotal, curl_off_t ulnow)
    {
        ISyncListener* pListener = state->m_listener;
        if (pListener->getProgressTotal() <= 0) {
            if (ultotal <= 0 && state->m_request.upload_data_size_bytes >= 0)
                ultotal = state->m_request.upload_data_size_bytes;
            int64_t total = ultotal + dltotal;
            if (total > 0)
                pListener->setProgressTotal(total);
        }
        int64_t so_far = ulnow + dlnow;
        pListener->onProgress(so_far);
        if (pListener->isCancelled()) {
            // returning non-zero signals libcurl to abort
            return 1;
        }

        return 0;
    }

    int debugCallback(CURL*,
        curl_infotype,
        char* data,
        size_t size,
        void*)
    {
        std::string smsg(data, size);
        OutputDebugStringA(smsg.c_str());
        return 0;
    }

    bool isRetryableError(CURLcode res)
    {
        switch (res) {
        case  CURLE_COULDNT_RESOLVE_PROXY:
        case  CURLE_COULDNT_RESOLVE_HOST:
        case  CURLE_COULDNT_CONNECT:
        case  CURLE_FTP_WEIRD_SERVER_REPLY:
        case  CURLE_REMOTE_ACCESS_DENIED:
        case  CURLE_FTP_ACCEPT_FAILED:
        case  CURLE_FTP_WEIRD_PASS_REPLY:
        case  CURLE_FTP_ACCEPT_TIMEOUT:
        case  CURLE_FTP_WEIRD_PASV_REPLY:
        case  CURLE_FTP_WEIRD_227_FORMAT:
        case  CURLE_FTP_CANT_GET_HOST:
        case  CURLE_HTTP2:
        case  CURLE_FTP_COULDNT_SET_TYPE:
        case  CURLE_PARTIAL_FILE:
        case  CURLE_FTP_COULDNT_RETR_FILE:
        case  CURLE_QUOTE_ERROR:
        case  CURLE_UPLOAD_FAILED:
        case  CURLE_OPERATION_TIMEDOUT:
        case  CURLE_FTP_PORT_FAILED:
        case  CURLE_FTP_COULDNT_USE_REST:
        case  CURLE_RANGE_ERROR:
        case  CURLE_HTTP_POST_ERROR:
        case  CURLE_SSL_CONNECT_ERROR:
        case  CURLE_BAD_DOWNLOAD_RESUME:
        case  CURLE_LDAP_CANNOT_BIND:
        case  CURLE_LDAP_SEARCH_FAILED:
        case  CURLE_GOT_NOTHING:
        case  CURLE_SEND_ERROR:
        case  CURLE_RECV_ERROR:
        case  CURLE_SSL_CIPHER:
        case  CURLE_SSL_CACERT:
        case  CURLE_LDAP_INVALID_URL:
        case  CURLE_USE_SSL_FAILED:
        case  CURLE_SEND_FAIL_REWIND:
        case  CURLE_LOGIN_DENIED:
        case  CURLE_TFTP_NOTFOUND:
        case  CURLE_TFTP_PERM:
        case  CURLE_REMOTE_DISK_FULL:
        case  CURLE_TFTP_ILLEGAL:
        case  CURLE_TFTP_UNKNOWNID:
        case  CURLE_REMOTE_FILE_EXISTS:
        case  CURLE_REMOTE_FILE_NOT_FOUND:
        case  CURLE_SSH:
        case  CURLE_SSL_SHUTDOWN_FAILED:
        case  CURLE_AGAIN:
        case  CURLE_SSL_ISSUER_ERROR:
        case  CURLE_FTP_PRET_FAILED:
        case  CURLE_RTSP_CSEQ_ERROR:
        case  CURLE_RTSP_SESSION_ERROR:
        case  CURLE_FTP_BAD_FILE_LIST:
            return true;
        default:
            return false;
        }
    }

    void ThrowSyncError(CURLcode curl_error_code, const std::string& error_message)
    {
        if (curl_error_code != CURLE_OK) {
            if (curl_error_code == CURLE_ABORTED_BY_CALLBACK) {
                throw SyncCancelException();
            }
            else if (isRetryableError(curl_error_code))
                throw SyncRetryableNetworkError(100119, UTF8Convert::UTF8ToWide<CString>(error_message));
            else
                throw SyncError(100119, UTF8Convert::UTF8ToWide<CString>(error_message));
        }
    }
}

CurlHttpConnection::CurlHttpConnection()
    : m_listener(nullptr)
{
    m_multi_handle = curl_multi_init();
}

CurlHttpConnection::~CurlHttpConnection()
{
    curl_multi_cleanup(m_multi_handle);
}

HttpResponse CurlHttpConnection::Request(const HttpRequest& request)
{
    Cleanup();
    m_state = std::make_unique<CurlOperationState>(request, m_listener);
    SetupEasyHandle();
    curl_multi_add_handle(m_multi_handle, m_state->m_easy_handle);

    int still_running;
    curl_multi_perform(m_multi_handle, &still_running);

    while (!m_state->m_finished_reading_headers) {
        RunLoop();
    }

    int http_status;
    curl_easy_getinfo(m_state->m_easy_handle, CURLINFO_RESPONSE_CODE, &http_status);
    HttpResponse response(http_status, m_state->m_response_headers);

    response.body.observable = rxcpp::observable<>::create<std::string>([this](rxcpp::subscriber<std::string> s) {

        m_state->m_body_subscriber = s;
        curl_easy_pause(m_state->m_easy_handle, CURLPAUSE_CONT);

        try {
            while (true) {
                if (!RunLoop())
                    break;
            }
            Cleanup();
            s.on_completed();
        }
        catch (const SyncException&) {
            Cleanup();
            s.on_error(std::current_exception());
        }
        });

    return response;
}

void CurlHttpConnection::SetupEasyHandle()
{
    curl_easy_setopt(m_state->m_easy_handle, CURLOPT_ERRORBUFFER, m_state->m_error_message.data());
    curl_easy_setopt(m_state->m_easy_handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(m_state->m_easy_handle, CURLOPT_POSTREDIR, CURL_REDIR_POST_ALL);

    //curl_easy_setopt(m_state->m_easy_handle, CURLOPT_VERBOSE, 1L);
    //curl_easy_setopt(m_state->m_easy_handle, CURLOPT_DEBUGFUNCTION, debugCallback);

    if (m_listener) {
        curl_easy_setopt(m_state->m_easy_handle, CURLOPT_PROGRESSDATA, m_state.get());
        curl_easy_setopt(m_state->m_easy_handle, CURLOPT_NOPROGRESS, 0);
        curl_easy_setopt(m_state->m_easy_handle, CURLOPT_XFERINFOFUNCTION, progressCallback);
        curl_easy_setopt(m_state->m_easy_handle, CURLOPT_NOPROGRESS, 0);
    }

    curl_easy_setopt(m_state->m_easy_handle, CURLOPT_URL, UTF8Convert::WideToUTF8(m_state->m_request.url).c_str());
    curl_easy_setopt(m_state->m_easy_handle, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(m_state->m_easy_handle, CURLOPT_WRITEDATA, m_state.get());
    curl_easy_setopt(m_state->m_easy_handle, CURLOPT_ACCEPT_ENCODING, ""); // Accept all encodings include gzip/deflate, CURL will automatically decompress
    curl_easy_setopt(m_state->m_easy_handle, CURLOPT_HEADERFUNCTION, headerCallback);
    curl_easy_setopt(m_state->m_easy_handle, CURLOPT_HEADERDATA, m_state.get());

    // Connection timeout after 10 seconds
    curl_easy_setopt(m_state->m_easy_handle, CURLOPT_CONNECTTIMEOUT, 10);
    // Timeout if less than 1 byte is transferred over a period of 3 minutes
    curl_easy_setopt(m_state->m_easy_handle, CURLOPT_LOW_SPEED_LIMIT, 1);
    curl_easy_setopt(m_state->m_easy_handle, CURLOPT_LOW_SPEED_TIME, 60 * 3);

    if (m_state->m_request.upload_data != NULL) {
        curl_easy_setopt(m_state->m_easy_handle, CURLOPT_READFUNCTION, readCallback);
        curl_easy_setopt(m_state->m_easy_handle, CURLOPT_READDATA, m_state.get());
        curl_easy_setopt(m_state->m_easy_handle, CURLOPT_SEEKFUNCTION, seekCallback);
        curl_easy_setopt(m_state->m_easy_handle, CURLOPT_SEEKDATA, m_state.get());
        curl_easy_setopt(m_state->m_easy_handle, CURLOPT_POST, 1L);
        curl_easy_setopt(m_state->m_easy_handle, CURLOPT_HTTPGET, 0L);
        if (m_state->m_request.method != HttpRequestMethod::HTTP_POST) {
            curl_easy_setopt(m_state->m_easy_handle, CURLOPT_CUSTOMREQUEST, HttpRequestMethodToString(m_state->m_request.method));
        }
        else {
            // Since we set CURLOPT_POST to 1 it will already
            // be POST so don't need custom verb
            curl_easy_setopt(m_state->m_easy_handle, CURLOPT_CUSTOMREQUEST, NULL);
        }

        if (m_state->m_request.upload_data_size_bytes == -1) {
            // no size specified so need to specify chunked encoding
            // since there will be no content-length header
            m_state->m_curl_request_headers = curl_slist_append(m_state->m_curl_request_headers, "Transfer-Encoding: chunked");
            curl_easy_setopt(m_state->m_easy_handle, CURLOPT_POSTFIELDSIZE, -1);
        }
        else {
            // Fixed size - this will set content-length header
            curl_easy_setopt(m_state->m_easy_handle, CURLOPT_POSTFIELDSIZE, (int)m_state->m_request.upload_data_size_bytes);
        }
    }
    else {
        curl_easy_setopt(m_state->m_easy_handle, CURLOPT_READFUNCTION, NULL);
        curl_easy_setopt(m_state->m_easy_handle, CURLOPT_READDATA, NULL);
        curl_easy_setopt(m_state->m_easy_handle, CURLOPT_POST, 0L);
        curl_easy_setopt(m_state->m_easy_handle, CURLOPT_HTTPGET, 1L);
        if (m_state->m_request.method != HttpRequestMethod::HTTP_GET) {
            curl_easy_setopt(m_state->m_easy_handle, CURLOPT_CUSTOMREQUEST, HttpRequestMethodToString(m_state->m_request.method));
        }
        else {
            // Since we set CURLOPT_HTTPGET request type is already
            // set to GET so don't need custom.
            curl_easy_setopt(m_state->m_easy_handle, CURLOPT_CUSTOMREQUEST, NULL);
        }
    }
    curl_easy_setopt(m_state->m_easy_handle, CURLOPT_HTTPHEADER, m_state->m_curl_request_headers);
}

void CurlHttpConnection::Cleanup()
{
    if (m_state) {
        curl_multi_remove_handle(m_multi_handle, m_state->m_easy_handle);
        m_state.reset();
    }
}

void CurlHttpConnection::CheckForErrors()
{
    int msgs_left;
    while (CURLMsg* msg = curl_multi_info_read(m_multi_handle, &msgs_left)) {
        if (msg->msg == CURLMSG_DONE) {
            if (msg->data.result != CURLE_OK) {
                ThrowSyncError(msg->data.result, m_state->m_error_message);
            }
        }
    }
}

bool CurlHttpConnection::RunLoop()
{
    int numfds;
    CURLMcode mc = curl_multi_wait(m_multi_handle, NULL, 0, 1000, &numfds);
    if (mc != CURLM_OK)
        throw SyncException(L"Network error");

    if (!numfds)
        Sleep(100);

    int still_running = 0;
    curl_multi_perform(m_multi_handle, &still_running);

    CheckForErrors();

    return still_running > 0;
}
