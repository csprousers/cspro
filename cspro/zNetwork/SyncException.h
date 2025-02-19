#pragma once

#include <zNetwork/zNetwork.h>
#include <zToolsO/CSProException.h>

class SystemMessageFormatter;


// --------------------------------------------------------------------------
// SyncException
//
// Derived from CSProException, the base class for sync-related exceptions.
// --------------------------------------------------------------------------

struct SyncException : public CSProException
{
    using CSProException::CSProException;
};


// --------------------------------------------------------------------------
// SyncError
// --------------------------------------------------------------------------

class SyncError : public SyncException
{
public:
    SyncError(int message_number, cs::string_sz message) : SyncException(message.c_str()),
                                                           m_messageNumber(message_number) { }

    SyncError(int message_number, int http_response_code, cs::string_sz message) : SyncException(message.c_str()),
                                                                                   m_messageNumber(message_number),
                                                                                   m_httpResponseCode(http_response_code) { }

    SyncError(int message_number, const std::exception& exception) : SyncException(exception.what()),
                                                                     m_messageNumber(message_number) { }

    SyncError(int message_number) : SyncException(FormatText("Sync Error: %d", message_number)),
                                    m_messageNumber(message_number) { }

    int GetErrorMessageNumber() const { return m_messageNumber; }

    const std::optional<int>& GetHttpResponseCode() const { return m_httpResponseCode; }

    // Throws a SyncError, or a subclass, based on the message number.
    template<typename... Args>
    [[noreturn]] static void ThrowByMessageNumber(int message_number, Args&&... args);

    // Throws a SyncError, or a subclass, based on the message number and HTTP response code.
    template<typename... Args>
    [[noreturn]] static void ThrowByMessageNumberAndHttpResponseCode(int message_number, int http_response_code, Args&&... args);

private:
    int m_messageNumber;
    std::optional<int> m_httpResponseCode;
};


// --------------------------------------------------------------------------
// SyncErrorFormatter
// --------------------------------------------------------------------------

class ZNETWORK_API SyncErrorFormatter
{
public:
    SyncErrorFormatter();
    ~SyncErrorFormatter();

    // Returns the exception's message, properly formatted if a SyncError.
    std::string GetFormattedError(const std::exception& exception);

private:
    std::unique_ptr<SystemMessageFormatter> m_systemMessageFormatter;
};



// --------------------------------------------------------------------------
// SyncCancelException
// --------------------------------------------------------------------------

struct SyncCancelException : public SyncException
{
    SyncCancelException() : SyncException("Sync canceled") { }
};


// --------------------------------------------------------------------------
// SyncRetryableNetworkError
// --------------------------------------------------------------------------

struct SyncRetryableNetworkError : public SyncError
{
    SyncRetryableNetworkError(int message_number, cs::string_sz message) : SyncError(message_number, message) { }
};


// --------------------------------------------------------------------------
// SyncConnectionError
// --------------------------------------------------------------------------

struct SyncConnectionError : public SyncError
{
    SyncConnectionError(const std::string& message) : SyncError(100101, message) { }

    template<typename... Args>
    SyncConnectionError(const char* formatter, Args const&... args) : SyncError(100101, FormatText(formatter, args...)) { }
};


// --------------------------------------------------------------------------
// SyncLoginDeniedError
// --------------------------------------------------------------------------

struct SyncLoginDeniedError : public SyncError
{
    SyncLoginDeniedError(int message_number) : SyncError(message_number) { }
};


// --------------------------------------------------------------------------
// SyncNotSupportedOperationError
// --------------------------------------------------------------------------

struct SyncNotSupportedOperationError : public SyncError
{
    SyncNotSupportedOperationError() : SyncError(100124, "The sync service does not support this functionality.") { }
};


// --------------------------------------------------------------------------
// SyncNotSupportedByServiceVersionError
// --------------------------------------------------------------------------

struct SyncNotSupportedByServiceVersionError : public SyncError
{
    SyncNotSupportedByServiceVersionError() : SyncError(100142, "The sync service does not support this functionality. Upgrade the service to the latest version.") { }
};


// --------------------------------------------------------------------------
// SyncSqliteError
// --------------------------------------------------------------------------

struct SyncSqliteError : public SyncError
{
    SyncSqliteError(int message_number) : SyncError(message_number) { }
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

template<typename... Args>
void SyncError::ThrowByMessageNumber(const int message_number, Args&&... args)
{
    switch( message_number )
    {
        case 100101: throw SyncConnectionError(std::forward<Args>(args)...);
        default:     throw SyncError(message_number, std::forward<Args>(args)...);
    }
}


template<typename... Args>
void SyncError::ThrowByMessageNumberAndHttpResponseCode(const int message_number, const int http_response_code, Args&&... args)
{
    switch( message_number )
    {
        case 100101: throw SyncConnectionError(std::forward<Args>(args)...);
        default:     throw SyncError(message_number, http_response_code, std::forward<Args>(args)...);
    }
}
