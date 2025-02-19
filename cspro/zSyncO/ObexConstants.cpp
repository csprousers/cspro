#include "stdafx.h"
#include "ObexConstants.h"

const unsigned int BLUETOOTH_PROTOCOL_VERSION = 1;
const wchar_t* OBEX_SYNC_DATA_MEDIA_TYPE = L"application / vnd.census.cspro.datasync + json";
const wchar_t* OBEX_DIRECTORY_LISTING_MEDIA_TYPE = L"application / vnd.census.cspro.dirlist + json";
const wchar_t* OBEX_BINARY_FILE_MEDIA_TYPE = L"application/octet-stream";
const wchar_t* OBEX_SYNC_APP_MEDIA_TYPE = L"application / vnd.census.cspro.appsync + octet-stream";
const wchar_t* OBEX_SYNC_MESSAGE_MEDIA_TYPE = L"application / vnd.census.cspro.message + json";
const wchar_t* OBEX_SYNC_PARADATA_SYNC_HANDSHAKE = L"application / vnd.census.cspro.paradata.handshake + text/plain";
const wchar_t* OBEX_SYNC_PARADATA_TYPE = L"application / vnd.census.cspro.paradata + octet-stream";

const GUID OBEX_SYNC_SERVICE_UUID = { 0xe6a9475e, 0x73da, 0x453a, 0xbf, 0x55, 0xba, 0x62, 0x76, 0x9c, 0xb8, 0xb4 };

// Don't use the Windows GUID structure for this one as it has a different byte order
// when sent over the wire
const unsigned char OBEX_FOLDER_BROWSING_UUID[16] = { 0xF9, 0xEC, 0x7B, 0xC4, 0x95, 0x3C, 0x11, 0xD2, 0x98, 0x4e, 0x52, 0x54, 0x00, 0xDC, 0x9E, 0x09 };


bool IsObexError(const ObexResponseCode code)
{
    return static_cast<int>(code) >= 0xC0;
}


const char* ObexResponseCodeToString(const ObexResponseCode code)
{
    switch( code )
    {
        case OBEX_CONTINUE:                         return "continue";
        case OBEX_OK:                               return "ok";
        case OBEX_CREATED:                          return "created";
        case OBEX_ACCEPTED:                         return "accepted";
        case OBEX_NON_AUTHORITATIVE_INFORMATION:    return "non authoritative information";
        case OBEX_NO_CONTENT:                       return "no content";
        case OBEX_RESET_CONTENT:                    return "reset content";
        case OBEX_PARTIAL_CONTENT:                  return "partial content";
        case OBEX_MUTLIPLE_CHOICES:                 return "mutliple choices";
        case OBEX_MOVED_PERMANENTLY:                return "moved permanently";
        case OBEX_MOVED_TEMPORARILY:                return "moved temporarily";
        case OBEX_SEE_OTHER:                        return "see other";
        case OBEX_NOT_MODIFIED:                     return "not modified";
        case OBEX_USE_PROXY:                        return "use proxy";
        case OBEX_BAD_REQUEST:                      return "bad request";
        case OBEX_UNAUTHORIZED:                     return "unauthorized";
        case OBEX_PAYMENT_REQUIRED:                 return "payment required";
        case OBEX_FORBIDDEN:                        return "forbidden";
        case OBEX_NOT_FOUND:                        return "not found";
        case OBEX_METHOD_NOT_ALLOWED:               return "method not allowed";
        case OBEX_NOT_ACCEPTABLE:                   return "not acceptable";
        case OBEX_PROXY_AUTHENTCATION_REQUIRED:     return "proxy authentcation required";
        case OBEX_REQUEST_TIMEOUT:                  return "request timeout";
        case OBEX_CONFLICT:                         return "conflict";
        case OBEX_GONE:                             return "gone";
        case OBEX_LENGTH_REQUIRED:                  return "length required";
        case OBEX_PRECONDITION_FAILED:              return "precondition failed";
        case OBEX_REQUESTED_ENTITY_TOO_LARGE:       return "requested entity too large";
        case OBEX_REQUESTED_URL_TOO_LARGE:          return "requested url too large";
        case OBEX_UNSUPPORTED_MEDIA_TYPE:           return "unsupported media type";
        case OBEX_INTERNAL_SERVER_ERROR:            return "internal server error";
        case OBEX_NOT_IMPLEMENTED:                  return "not implemented";
        case OBEX_BAD_GATEWAY:                      return "bad gateway";
        case OBEX_SERVICE_UNAVAILABLE:              return "service unavailable";
        case OBEX_GATEWAY_TIMEOUT:                  return "gateway timeout";
        case OBEX_HTTP_VERSION_NOT_SUPPORTED:       return "http version not supported";
        case OBEX_DATABASE_FULL:                    return "database full";
        case OBEX_DATABASE_LOCKED:                  return "database locked";
        default:                                    return ReturnProgrammingError("");
    }    
}
