#include "stdafx.h"
#include "CurlWrapper.h"
#include "HeaderList.h"
#include <sstream>


constexpr bool EnableVerboseLogging = false;


CurlWrapper::CurlWrapper(const Type type, void*& curl)
    :   m_type(type),
        m_curl(curl)
{
    m_curl = curl_easy_init();
    ASSERT(m_curl != nullptr);

    curl_easy_setopt(m_curl, CURLOPT_ERRORBUFFER, m_errorBuffer);

    if constexpr(EnableVerboseLogging)
    {
        curl_easy_setopt(m_curl, CURLOPT_VERBOSE, 1);
        curl_easy_setopt(m_curl, CURLOPT_DEBUGFUNCTION, DebugCallback);
    }
}


CurlWrapper::~CurlWrapper()
{
    curl_easy_cleanup(m_curl);
}


curl_slist* CurlWrapper::CreateCurlHeaders(const HeaderList& headers)
{
    curl_slist* curl_headers = nullptr;

    for( const std::string& header : headers.GetHeaders() )
        curl_headers = curl_slist_append(curl_headers, header.c_str());

    return curl_headers;
}


bool CurlWrapper::IsRetryableError(const CURLcode result) const
{
    switch( result )
    {
        case CURLE_COULDNT_RESOLVE_PROXY:
        case CURLE_COULDNT_RESOLVE_HOST:
        case CURLE_COULDNT_CONNECT:
        case CURLE_FTP_WEIRD_SERVER_REPLY:
        case CURLE_REMOTE_ACCESS_DENIED:
        case CURLE_FTP_ACCEPT_FAILED:
        case CURLE_FTP_WEIRD_PASS_REPLY:
        case CURLE_FTP_ACCEPT_TIMEOUT:
        case CURLE_FTP_WEIRD_PASV_REPLY:
        case CURLE_FTP_WEIRD_227_FORMAT:
        case CURLE_FTP_CANT_GET_HOST:
        case CURLE_HTTP2:
        case CURLE_FTP_COULDNT_SET_TYPE:
        case CURLE_PARTIAL_FILE:
        case CURLE_FTP_COULDNT_RETR_FILE:
        case CURLE_QUOTE_ERROR:
        case CURLE_UPLOAD_FAILED:
        case CURLE_OPERATION_TIMEDOUT:
        case CURLE_FTP_PORT_FAILED:
        case CURLE_FTP_COULDNT_USE_REST:
        case CURLE_RANGE_ERROR:
        case CURLE_HTTP_POST_ERROR:
        case CURLE_SSL_CONNECT_ERROR:
        case CURLE_BAD_DOWNLOAD_RESUME:
        case CURLE_LDAP_CANNOT_BIND:
        case CURLE_LDAP_SEARCH_FAILED:
        case CURLE_GOT_NOTHING:
        case CURLE_SEND_ERROR:
        case CURLE_RECV_ERROR:
        case CURLE_SSL_CIPHER:
        case CURLE_SSL_CACERT:
        case CURLE_LDAP_INVALID_URL:
        case CURLE_USE_SSL_FAILED:
        case CURLE_SEND_FAIL_REWIND:
        case CURLE_TFTP_NOTFOUND:
        case CURLE_TFTP_PERM:
        case CURLE_REMOTE_DISK_FULL:
        case CURLE_TFTP_ILLEGAL:
        case CURLE_TFTP_UNKNOWNID:
        case CURLE_REMOTE_FILE_EXISTS:
        case CURLE_REMOTE_FILE_NOT_FOUND:
        case CURLE_SSH:
        case CURLE_SSL_SHUTDOWN_FAILED:
        case CURLE_AGAIN:
        case CURLE_SSL_ISSUER_ERROR:
        case CURLE_FTP_PRET_FAILED:
        case CURLE_RTSP_CSEQ_ERROR:
        case CURLE_RTSP_SESSION_ERROR:
        case CURLE_FTP_BAD_FILE_LIST:
            return true;

        case CURLE_LOGIN_DENIED:
            return ( m_type == Type::Http );

        default:
            return false;
    }
}


void CurlWrapper::ThrowSyncException(const CURLcode result, const int message_number) const
{
    ASSERT(result != CURLE_OK);

    if( result == CURLE_ABORTED_BY_CALLBACK )
        throw SyncCancelException();

    if( result == CURLE_LOGIN_DENIED )
        throw SyncLoginDeniedError(100126);

    if( IsRetryableError(result) )
        throw SyncRetryableNetworkError(message_number, m_errorBuffer);

    SyncError::ThrowByMessageNumber(message_number, m_errorBuffer);
}


int CurlWrapper::DebugCallback(CURL* /*handle*/, const curl_infotype /*type*/, char* const data, const size_t size, void* /*clientp*/)
{
    OutputDebugString(TC::ToWide(data, size).c_str());
    return 0;
}


size_t CurlWrapper::WriteToOutputStreamCallback(char* const buffer, const size_t size, const size_t nitems, std::ostream* const output_stream)
{
    const size_t write_size = size * nitems;
    output_stream->write(buffer, write_size);
    return write_size;
}


size_t CurlWrapper::WriteToNothingCallback(char* /*buffer*/, const size_t size, const size_t nitems, void*)
{
    return size * nitems;
}


size_t CurlWrapper::ReadFromInputStreamDataCallback(char* const buffer, const size_t size, const size_t nitems, InputStreamData* const input_stream_data)
{
    const size_t read_size = size * nitems;

    if( read_size == 0 || input_stream_data->stream.eof() )
        return 0;

    if( input_stream_data->stream.fail() || input_stream_data->stream.bad() )
    {
        input_stream_data->read_error = true;
        return CURL_READFUNC_ABORT;
    }

    input_stream_data->stream.read(buffer, read_size);

    return static_cast<size_t>(input_stream_data->stream.gcount());
}


void CurlWrapper::SetOptionsForProgressHandling()
{
    if( m_syncListener == nullptr )
        return;

    curl_easy_setopt(m_curl, CURLOPT_PROGRESSDATA, m_syncListener.get());
    curl_easy_setopt(m_curl, CURLOPT_NOPROGRESS, 0);
}


int CurlWrapper::ProgressCallback(SyncListener* const sync_listener, const curl_off_t dltotal, const curl_off_t dlnow, const curl_off_t ultotal, const curl_off_t ulnow)
{
    ASSERT(sync_listener != nullptr);

    if( sync_listener->GetProgressTotal() <= 0 )
    {
        const int64_t total = ultotal + dltotal;

        if( total > 0 )
            sync_listener->SetProgressTotal(total);
    }

    const int64_t so_far = ulnow + dlnow;
    sync_listener->Progress(so_far);

    // returning non-zero signals libcurl to abort
    if( sync_listener->IsCanceled() )
        return 1;

    return 0;
}
