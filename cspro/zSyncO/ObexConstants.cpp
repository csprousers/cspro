#include "stdafx.h"
#include "ObexConstants.h"

SYNC_API const unsigned int BLUETOOTH_PROTOCOL_VERSION = 1;
SYNC_API const wchar_t* OBEX_SYNC_DATA_MEDIA_TYPE = L"application / vnd.census.cspro.datasync + json";
SYNC_API const wchar_t* OBEX_DIRECTORY_LISTING_MEDIA_TYPE = L"application / vnd.census.cspro.dirlist + json";
SYNC_API const wchar_t* OBEX_BINARY_FILE_MEDIA_TYPE = L"application/octet-stream";
SYNC_API const wchar_t* OBEX_SYNC_APP_MEDIA_TYPE = L"application / vnd.census.cspro.appsync + octet-stream";
SYNC_API const wchar_t* OBEX_SYNC_MESSAGE_MEDIA_TYPE = L"application / vnd.census.cspro.message + json";
SYNC_API const wchar_t* OBEX_SYNC_PARADATA_SYNC_HANDSHAKE = L"application / vnd.census.cspro.paradata.handshake + text/plain";
SYNC_API const wchar_t* OBEX_SYNC_PARADATA_TYPE = L"application / vnd.census.cspro.paradata + octet-stream";

SYNC_API const GUID OBEX_SYNC_SERVICE_UUID = { 0xe6a9475e, 0x73da, 0x453a, 0xbf, 0x55, 0xba, 0x62, 0x76, 0x9c, 0xb8, 0xb4 };

// Don't use the Windows GUID structure for this one as it has a different byte order
// when sent over the wire
SYNC_API const unsigned char OBEX_FOLDER_BROWSING_UUID[16] = { 0xF9, 0xEC, 0x7B, 0xC4, 0x95, 0x3C, 0x11, 0xD2, 0x98, 0x4e, 0x52, 0x54, 0x00, 0xDC, 0x9E, 0x09 };

SYNC_API bool IsObexError(ObexResponseCode code)
{
    return (int) code >= 0xC0;
}

CString ObexResponseCodeToString(ObexResponseCode code)
{
    switch (code) {
    case OBEX_CONTINUE:
        return CString("continue");
    case OBEX_OK:
        return CString("ok");
    case OBEX_CREATED:
        return CString("created");
    case OBEX_ACCEPTED:
        return CString("accepted");
    case OBEX_NON_AUTHORITATIVE_INFORMATION:
        return CString("non authoritative information");
    case OBEX_NO_CONTENT:
        return CString("no content");
    case OBEX_RESET_CONTENT:
        return CString("reset content");
    case OBEX_PARTIAL_CONTENT:
        return CString("partial content");
    case OBEX_MUTLIPLE_CHOICES:
        return CString("mutliple choices");
    case OBEX_MOVED_PERMANENTLY:
        return CString("moved permanently");
    case OBEX_MOVED_TEMPORARILY:
        return CString("moved temporarily");
    case OBEX_SEE_OTHER:
        return CString("see other");
    case OBEX_NOT_MODIFIED:
        return CString("not modified");
    case OBEX_USE_PROXY:
        return CString("use proxy");
    case OBEX_BAD_REQUEST:
        return CString("bad request");
    case OBEX_UNAUTHORIZED:
        return CString("unauthorized");
    case OBEX_PAYMENT_REQUIRED:
        return CString("payment required");
    case OBEX_FORBIDDEN:
        return CString("forbidden");
    case OBEX_NOT_FOUND:
        return CString("not found");
    case OBEX_METHOD_NOT_ALLOWED:
        return CString("method not allowed");
    case OBEX_NOT_ACCEPTABLE:
        return CString("not acceptable");
    case OBEX_PROXY_AUTHENTCATION_REQUIRED:
        return CString("proxy authentcation required");
    case OBEX_REQUEST_TIMEOUT:
        return CString("request timeout");
    case OBEX_CONFLICT:
        return CString("conflict");
    case OBEX_GONE:
        return CString("gone");
    case OBEX_LENGTH_REQUIRED:
        return CString("length required");
    case OBEX_PRECONDITION_FAILED:
        return CString("precondition failed");
    case OBEX_REQUESTED_ENTITY_TOO_LARGE:
        return CString("requested entity too large");
    case OBEX_REQUESTED_URL_TOO_LARGE:
        return CString("requested url too large");
    case OBEX_UNSUPPORTED_MEDIA_TYPE:
        return CString("unsupported media type");
    case OBEX_INTERNAL_SERVER_ERROR:
        return CString("internal server error");
    case OBEX_NOT_IMPLEMENTED:
        return CString("not implemented");
    case OBEX_BAD_GATEWAY:
        return CString("bad gateway");
    case OBEX_SERVICE_UNAVAILABLE:
        return CString("service unavailable");
    case OBEX_GATEWAY_TIMEOUT:
        return CString("gateway timeout");
    case OBEX_HTTP_VERSION_NOT_SUPPORTED:
        return CString("http version not supported");
    case OBEX_DATABASE_FULL:
        return CString("database full");
    case OBEX_DATABASE_LOCKED:
        return CString("database locked");
    }
    return CString();
}
