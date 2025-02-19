#include "stdafx.h"
#include "CurlHttpConnection.h"
#include "CurlWrapper.h"


class CurlOperationState
{
public:
    CurlOperationState(HttpRequest request, std::shared_ptr<SyncListener> sync_listener)
        :   m_request(std::move(request)),
            m_curlWrapper(CurlWrapper::Type::Http, m_easy_handle),
            m_curl_request_headers(CurlWrapper::CreateCurlHeaders(m_request.headers)),
            m_finished_reading_headers(false),
            m_syncListener(std::move(sync_listener)),
            m_inCleanup(false)
    {
    }

    ~CurlOperationState()
    {
        if( m_curl_request_headers != nullptr )
            curl_slist_free_all(m_curl_request_headers);
    }

    HttpRequest m_request;
    CURL* m_easy_handle;
    CurlWrapper m_curlWrapper;
    std::optional<rxcpp::subscriber<std::string>> m_body_subscriber;
    curl_slist* m_curl_request_headers;
    HeaderList m_response_headers;
    bool m_finished_reading_headers;
    std::shared_ptr<SyncListener> m_syncListener;
    bool m_inCleanup;
};


namespace
{
    bool IsContinueHeader(const std::string_view header_sv)
    {
        return ( header_sv.length() > 12 &&
                 header_sv.substr(0, 5) == "HTTP/" &&
                 header_sv.substr(header_sv.length() - 12) == "100 Continue" );
    }

    int writeCallback(char* const data, const size_t size, const size_t nmemb, CurlOperationState* const state)
    {
        if( state->m_body_subscriber.has_value() )
        {
            const size_t write_size = size * nmemb;
            std::string s(data, write_size);
            OutputDebugStringA(s.c_str());
            (*state->m_body_subscriber).on_next(std::move(s));
            return write_size;
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
        std::string header(data, size * nmemb);
        SO::MakeTrimRight(header); // remove the trailing crlf
        if (header.empty()) {
            // An empty header signals end of headers, but not if it is after a "HTTP/1.1 100 Continue" which comes before reading input data
            if (!state->m_response_headers.GetHeaders().empty()) {
                const std::string& last_header = state->m_response_headers.GetHeaders().back();
                if (!IsContinueHeader(last_header))
                    state->m_finished_reading_headers = true;
            }
        }
        else {
            state->m_response_headers.Add(std::move(header));
        }
        return size * nmemb;
    }

    int progressCallback(CurlOperationState* state, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
    {
        // don't update the progress when this object is being destroyed in CurlHttpConnection::Cleanup
        if (state->m_inCleanup) {
            return 0;
        }

        SyncListener* const sync_listener = state->m_syncListener.get();

        if( sync_listener == nullptr ) {
            return 0;
        }

        if (sync_listener->GetProgressTotal() <= 0) {
            if (ultotal <= 0 && state->m_request.upload_data_size_bytes >= 0)
                ultotal = state->m_request.upload_data_size_bytes;
            int64_t total = ultotal + dltotal;
            if (total > 0)
                sync_listener->SetProgressTotal(total);
        }
        int64_t so_far = ulnow + dlnow;
        sync_listener->Progress(so_far);
        if (sync_listener->IsCanceled()) {
            // returning non-zero signals libcurl to abort
            return 1;
        }

        return 0;
    }
}


CurlHttpConnection::CurlHttpConnection()
    :   m_multiHandle(curl_multi_init())
{
}


CurlHttpConnection::~CurlHttpConnection()
{
    // clean up any pending requests
    for( const std::unique_ptr<CurlOperationState>& state : m_states )
    {
        if( state != nullptr )
        {
            try
            {
                Cleanup(*state);
            }
            catch(...) { ASSERT(false); }
        }
    }

    curl_multi_cleanup(m_multiHandle);
}


HttpResponse CurlHttpConnection::Request(const HttpRequest& request)
{
    size_t state_index;
    CurlOperationState* state;

    // lock
    {
        // see if we can reuse a state slot, or if we must create a new one
        std::lock_guard<std::mutex> lock(m_statesMutex);

        std::unique_ptr<CurlOperationState>* state_ptr = m_states.data();

        for( state_index = 0; state_index < m_states.size(); ++state_index, ++state_ptr )
        {
            if( *state_ptr == nullptr )
                break;
        }

        if( state_index == m_states.size() )
            state_ptr = &m_states.emplace_back(nullptr);

        ASSERT(state_index < m_states.size() && state_ptr >= &m_states.front() && state_ptr <= &m_states.back());

        *state_ptr = std::make_unique<CurlOperationState>(request, m_syncListener);

        state = state_ptr->get();
    }

    SetupEasyHandle(*state);
    curl_multi_add_handle(m_multiHandle, state->m_easy_handle);

    int still_running;
    curl_multi_perform(m_multiHandle, &still_running);

    while( !state->m_finished_reading_headers )
        RunLoop(*state);

    int http_status;
    curl_easy_getinfo(state->m_easy_handle, CURLINFO_RESPONSE_CODE, &http_status);
    HttpResponse response(http_status, state->m_response_headers);

    response.body.observable = rxcpp::observable<>::create<std::string>(
        [state_index, state, this](rxcpp::subscriber<std::string> s)
        {
            state->m_body_subscriber = s;
            curl_easy_pause(state->m_easy_handle, CURLPAUSE_CONT);

            std::exception_ptr caught_exception;

            try
            {
                while( RunLoop(*state) )
                {
                }

                s.on_completed();
            }

            catch( const SyncException& )
            {
                caught_exception = std::current_exception();
            }

            Cleanup(*state);

            // lock
            {
                std::lock_guard<std::mutex> lock(m_statesMutex);
                m_states[state_index].reset();
            }

            if( caught_exception )
                s.on_error(caught_exception);
        });

    return response;
}


void CurlHttpConnection::SetupEasyHandle(CurlOperationState& state) const
{
    curl_easy_setopt(state.m_easy_handle, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(state.m_easy_handle, CURLOPT_POSTREDIR, CURL_REDIR_POST_ALL);

    if( m_syncListener != nullptr )
    {
        curl_easy_setopt(state.m_easy_handle, CURLOPT_PROGRESSDATA, &state);
        curl_easy_setopt(state.m_easy_handle, CURLOPT_NOPROGRESS, 0);
        curl_easy_setopt(state.m_easy_handle, CURLOPT_XFERINFOFUNCTION, progressCallback);
        curl_easy_setopt(state.m_easy_handle, CURLOPT_NOPROGRESS, 0);
    }

    curl_easy_setopt(state.m_easy_handle, CURLOPT_URL, state.m_request.url.c_str());
    curl_easy_setopt(state.m_easy_handle, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(state.m_easy_handle, CURLOPT_WRITEDATA, &state);
    curl_easy_setopt(state.m_easy_handle, CURLOPT_ACCEPT_ENCODING, ""); // Accept all encodings include gzip/deflate, CURL will automatically decompress
    curl_easy_setopt(state.m_easy_handle, CURLOPT_HEADERFUNCTION, headerCallback);
    curl_easy_setopt(state.m_easy_handle, CURLOPT_HEADERDATA, &state);

    // Connection timeout after 10 seconds
    curl_easy_setopt(state.m_easy_handle, CURLOPT_CONNECTTIMEOUT, 10);

    // Timeout if less than 1 byte is transferred over a period of 3 minutes
    curl_easy_setopt(state.m_easy_handle, CURLOPT_LOW_SPEED_LIMIT, 1);
    curl_easy_setopt(state.m_easy_handle, CURLOPT_LOW_SPEED_TIME, 60 * 3);

    if( state.m_request.upload_data != nullptr)
    {
        curl_easy_setopt(state.m_easy_handle, CURLOPT_READFUNCTION, readCallback);
        curl_easy_setopt(state.m_easy_handle, CURLOPT_READDATA, &state);
        curl_easy_setopt(state.m_easy_handle, CURLOPT_SEEKFUNCTION, seekCallback);
        curl_easy_setopt(state.m_easy_handle, CURLOPT_SEEKDATA, &state);
        curl_easy_setopt(state.m_easy_handle, CURLOPT_POST, 1);
        curl_easy_setopt(state.m_easy_handle, CURLOPT_HTTPGET, 0);

        if( state.m_request.method != HttpRequestMethod::HTTP_POST )
        {
            curl_easy_setopt(state.m_easy_handle, CURLOPT_CUSTOMREQUEST, HttpRequestMethodToString(state.m_request.method));
        }

        else
        {
            // Since we set CURLOPT_POST to 1 it will already
            // be POST so don't need custom verb
            curl_easy_setopt(state.m_easy_handle, CURLOPT_CUSTOMREQUEST, nullptr);
        }

        if( state.m_request.upload_data_size_bytes == -1 )
        {
            // no size specified so need to specify chunked encoding
            // since there will be no content-length header
            state.m_curl_request_headers = curl_slist_append(state.m_curl_request_headers, "Transfer-Encoding: chunked");
            curl_easy_setopt(state.m_easy_handle, CURLOPT_POSTFIELDSIZE, -1);
        }

        else
        {
            // Fixed size - this will set content-length header
            curl_easy_setopt(state.m_easy_handle, CURLOPT_POSTFIELDSIZE, static_cast<int>(state.m_request.upload_data_size_bytes));
        }
    }

    else
    {
        curl_easy_setopt(state.m_easy_handle, CURLOPT_READFUNCTION, nullptr);
        curl_easy_setopt(state.m_easy_handle, CURLOPT_READDATA, nullptr);
        curl_easy_setopt(state.m_easy_handle, CURLOPT_POST, 0);
        curl_easy_setopt(state.m_easy_handle, CURLOPT_HTTPGET, 1);

        if( state.m_request.method != HttpRequestMethod::HTTP_GET )
        {
            curl_easy_setopt(state.m_easy_handle, CURLOPT_CUSTOMREQUEST, HttpRequestMethodToString(state.m_request.method));
        }

        else
        {
            // Since we set CURLOPT_HTTPGET request type is already
            // set to GET so don't need custom.
            curl_easy_setopt(state.m_easy_handle, CURLOPT_CUSTOMREQUEST, nullptr);
        }
    }

    curl_easy_setopt(state.m_easy_handle, CURLOPT_HTTPHEADER, state.m_curl_request_headers);
}


void CurlHttpConnection::Cleanup(CurlOperationState& state) const
{
    state.m_inCleanup = true;
    curl_multi_remove_handle(m_multiHandle, state.m_easy_handle);
}


bool CurlHttpConnection::RunLoop(CurlOperationState& state) const
{
    int numfds;
    const CURLMcode mc = curl_multi_wait(m_multiHandle, nullptr, 0, 1000, &numfds);

    if( mc != CURLM_OK )
        throw SyncException("Network error");

    if( numfds == 0 )
        Sleep(100);

    int running_handles;
    curl_multi_perform(m_multiHandle, &running_handles);

    int msgs_in_queue;
    CURLMsg* msg;

    while( ( msg = curl_multi_info_read(m_multiHandle, &msgs_in_queue) ) != nullptr )
    {
        if( msg->msg == CURLMSG_DONE )
        {
            if( msg->data.result != CURLE_OK )
                state.m_curlWrapper.ThrowSyncException(msg->data.result, 100101);

            // signal that this state's transfer is complete
            if( msg->easy_handle == state.m_easy_handle )
                return false;
        }
    }

    return ( running_handles > 0 );
}
