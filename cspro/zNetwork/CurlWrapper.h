#pragma once

#include <external/curl/include/curl/curl.h>

class HeaderList;
class SyncListener;


// CurlWrapper is a class that contains shared functionality used by CurlFtpConnection and CurlHttpConnection

class CurlWrapper
{
    // many cURL variadic arguments require longs, so ensure that int and long are the same
    // so that arguments do not need to be specified as, e.g., 1L
    static_assert(sizeof(int) == sizeof(long));

public:
    enum Type { Ftp, Http };

    // calls curl_easy_init and sets some options
    CurlWrapper(Type type, void*& curl);

    // calls curl_easy_cleanup
    ~CurlWrapper();


    // --------------------------------------------------------------------------
    // headers
    // --------------------------------------------------------------------------

    static curl_slist* CreateCurlHeaders(const HeaderList& headers);


    // --------------------------------------------------------------------------
    // error handling
    // --------------------------------------------------------------------------

    bool IsRetryableError(CURLcode result) const;

    // throws an exception based on the result:
    // CURLE_ABORTED_BY_CALLBACK -> SyncCancelException
    // CURLE_LOGIN_DENIED        -> SyncLoginDeniedError
    // a retryable error         -> SyncRetryableNetworkError
    // else                      -> SyncError error based on whether the result is a retryable error
    [[noreturn]] void ThrowSyncException(CURLcode result, int message_number) const;


    // --------------------------------------------------------------------------
    // callbacks
    // --------------------------------------------------------------------------
    
    static int DebugCallback(CURL* handle, curl_infotype type, char* data, size_t size, void* clientp);

    static size_t WriteToOutputStreamCallback(char* buffer, size_t size, size_t nitems, std::ostream* output_stream);
    static size_t WriteToNothingCallback(char* buffer, size_t size, size_t nitems, void*);

    struct InputStreamData { std::istream &stream; bool read_error = false; };
    static size_t ReadFromInputStreamDataCallback(char* buffer, size_t size, size_t nitems, InputStreamData* input_stream_data);


    // --------------------------------------------------------------------------
    // progress handling
    // --------------------------------------------------------------------------

    void SetSyncListener(std::shared_ptr<SyncListener> sync_listener) { m_syncListener = std::move(sync_listener); }

    void SetOptionsForProgressHandling();

    static int ProgressCallback(SyncListener* sync_listener, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);

private:
    Type m_type;
    CURL*& m_curl;
    char m_errorBuffer[CURL_ERROR_SIZE];
    std::shared_ptr<SyncListener> m_syncListener;
};
